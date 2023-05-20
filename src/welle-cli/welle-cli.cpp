#include <sys/stat.h>
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <set>
#include <utility>
#include <cstdio>
#include <cctype>
#include <unistd.h>
#ifdef HAVE_SOAPYSDR
#  include "soapy_sdr.h"
#endif
#include "backend/radio-receiver.h"
#include "input/input_factory.h"
#include "various/channels.h"
#include "libs/json.hpp"

#include <sys/socket.h>
#include <arpa/inet.h> // inet_addr

using namespace std;
using namespace nlohmann;

const std::string WHITESPACE = " \n\r\t\f\v";

const bool DEBUG = true;

std::string ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string &s)
{
    return rtrim(ltrim(s));
}

vector<string> split(string s, string delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

int sendJsonToMulticast(json& json, int fd, sockaddr_in addr)
{
    if (fd == 0) {
        cout << "creation socket" << endl;
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) {
            perror("socket");
            return false;
        } else {
            cout << "socket créée " << fd << endl;
        }
    }

    string json_str = json.dump() + "\n";
    int nbytes = sendto(fd, json_str.c_str(), json_str.size(), 0, (struct sockaddr*) &addr, sizeof(addr));
    if (nbytes < 0) {
        cout << "udp error: " << errno << endl;
        perror("sendto");
        return false;
    } else {
        //cout << "udp packet sent with " << nbytes << " bytes" << endl;
    }

    return fd;
}

int sendPcmToMulticast(std::vector<int16_t>& data, int fd, sockaddr_in addr)
{
    if (fd == 0) {
        cout << "creation socket" << endl;
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) {
            perror("socket");
            return false;
        } else {
            cout << "socket créée " << fd << endl;
        }
    }

    // split en nbParts éléments de partSize octets
    const int partSize = 1280; // 3 paquets UDP par onNewAudio

    for (size_t i = 0; i < data.size(); i += partSize) {
        auto last = std::min(data.size(), i + partSize);
        std::vector<int16_t> chunk(data.begin() + i, data.begin() + last);
        int nbytes = sendto(fd, chunk.data(), chunk.size() * sizeof(uint16_t), 0, (struct sockaddr*) &addr, sizeof(addr));
        if (nbytes < 0) {
            cout << "udp error: " << errno << endl;
            perror("sendto");
            return false;
        } else {
            //cout << "udp packet sent with " << nbytes << " bytes" << endl;
        }
    }

    return fd;
}

class WavProgrammeHandler: public ProgrammeHandlerInterface
{
    public:
        WavProgrammeHandler(uint32_t SId, const std::string& fileprefix, const char* mcastGroup, int mcastPort) :
            SId(SId),
            filePrefix(fileprefix) {
            stringstream _serviceIdStr;
            _serviceIdStr << hex << SId;
            serviceIdStr = _serviceIdStr.str();

            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr(mcastGroup);
            addr.sin_port = htons(mcastPort);

            memset(&addr2, 0, sizeof(addr2));
            addr2.sin_family = AF_INET;
            addr2.sin_addr.s_addr = inet_addr(mcastGroup);
            addr2.sin_port = htons(mcastPort + 1);
        }

        WavProgrammeHandler(const WavProgrammeHandler& other) = delete;
        WavProgrammeHandler& operator=(const WavProgrammeHandler& other) = delete;
        WavProgrammeHandler(WavProgrammeHandler&& other) = default;
        WavProgrammeHandler& operator=(WavProgrammeHandler&& other) = default;

        virtual void onFrameErrors(int frameErrors) override
        {
            (void)frameErrors;
        }

        /**
         * A new audio frame has been decoded to raw pcm
         * at 48kHz, duration is 20ms. if stereo and 16 bits: 1920 samples: 3840 bytes
         * we write into the .pcm file
         */
        virtual void onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, const string& mode) override
        {
            unsigned long int timestamp = time(NULL);
            json j;

            j["newAudio"] = {
                "size", audioData.size(), // in bytes
                "sampleRate", sampleRate,
                "mode", mode,
                "serviceId", serviceIdStr,
                "ts", timestamp
            };

            if (audioData.size() == 0) {
                cout << "audioData vide" << endl;
                return;
            }

            // sent to udp multicast
            fd = sendPcmToMulticast(audioData, fd, addr);

            // write to file
            /*
            string filename = filePrefix + ".pcm";
            FILE *file = fopen(filename.c_str(), "ab");
            cout << "fwrite " << audioData.size() << " bytes to " << filename << endl;
            fwrite(audioData.data(), sizeof(short), audioData.size(), file);
            fclose(file);
            */
        }

        virtual void onRsErrors(bool uncorrectedErrors, int numCorrectedErrors) override
        {
            (void)uncorrectedErrors; (void)numCorrectedErrors;
        }

        virtual void onAacErrors(int aacErrors) override
        {
            (void)aacErrors;
        }

        virtual void onNewDynamicLabel(const std::string& label) override
        {
            ofstream file;
            string filename = filePrefix + ".ndjson";
            unsigned long int timestamp = time(NULL);

            file.open(filename, std::ios_base::app);

            json j;
            j["dls"] = {
                {"value", trim(label)},
                {"ts", timestamp},
                {"serviceId", serviceIdStr}
            };
            file << j << endl;
            file.close();

            cout << j << endl;

            fd2 = sendJsonToMulticast(j, fd2, addr2);
        }

        // new MOT picture has been received
        virtual void onMOT(const mot_file_t& mot_file) override
        {
            string extension;
            switch (mot_file.content_sub_type) {
                case 0x01:
                    extension = "jpg";
                    break;
                case 0x03:
                    extension = "png";
                    break;
                default:
                    cerr << "MOT:content_sub_type unknown" << endl;
                    return;
            }

            ofstream file_mot, file_txt;
            unsigned long int timestamp = time(NULL);

            uint32_t current_mot_size = mot_file.data.size();
            if (current_mot_size == last_mot_size) {
                // detect duplicate (base on same file size)
                json j;
                j["motBypass"] = {
                    {"msg", "duplicate"},
                    {"size", last_mot_size},
                    {"serviceId", serviceIdStr},
                    {"ts", timestamp}
                };

                cout << j << endl;
                fd2 = sendJsonToMulticast(j, fd2, addr2);
                return;
            }
            last_mot_size = current_mot_size;

            // write the MOT picture
            string filename_mot = filePrefix + "-" + std::to_string(timestamp) + "." + extension;
            file_mot.open(filename_mot);
            std::stringstream ss;
            for (auto it = mot_file.data.begin(); it != mot_file.data.end(); it++) {
                ss << *it;
            }
            file_mot << ss.str();
            file_mot.close();

            // write MOT metadata
            string filename_txt = filePrefix + ".ndjson";
            file_txt.open(filename_txt, std::ios_base::app);

            json j;
            j["mot"] = {
                {"file", filename_mot.substr(filename_mot.find_last_of("/\\") + 1)},
                {"content_name", mot_file.content_name},
                {"click_through_url", mot_file.click_through_url},
                {"category_title", mot_file.category_title},
                {"serviceId", serviceIdStr},
                {"ts", timestamp}
            };
            cout << j << endl;
            file_txt << j << endl;
            file_txt.close();
            fd2 = sendJsonToMulticast(j, fd2, addr2);
        }

        virtual void onPADLengthError(size_t announced_xpad_len, size_t xpad_len) override
        {
            unsigned long int timestamp = time(NULL);
            json j;

            j["padError"] = {
                {"msg", "X-PAD length mismatch"},
                {"expected", announced_xpad_len},
                {"got", xpad_len},
                {"serviceId", serviceIdStr},
                {"ts", timestamp}
            };

            cout << j << endl;
            fd2 = sendJsonToMulticast(j, fd2, addr2);
        }

    private:
        uint32_t last_mot_size = 0; // store the last MOT file size in bytes
        uint32_t SId;
        string serviceIdStr;
        string filePrefix;
        int fd = 0; // pcm
        int fd2 = 0; // metadata
        struct sockaddr_in addr; // pcm
        struct sockaddr_in addr2; // metadata
};

class RadioInterface : public RadioControllerInterface
{
    public:
        // Signal on Noise Ratio
        virtual void onSNR(float snr) override
        {
            unsigned long int timestamp = time(NULL);
            json j;

            j["snr"] = {
                {"value", snr},
                {"channel", channel},
                {"frequency", frequency},
                {"ts", timestamp}
            };

            if (last_snr != j) {
                cout << j << endl;
                last_snr = j;
            }
        }

        virtual void onFrequencyCorrectorChange(int /*fine*/, int /*coarse*/) override
        {
        }

        virtual void onSyncChange(char isSync) override
        {
            synced = isSync;
        }

        virtual void onSignalPresence(bool /*isSignal*/) override
        {
        }

        virtual void onServiceDetected(uint32_t sId) override
        {
            unsigned long int timestamp = time(NULL);
            json j;

            serviceId = sId;
            stringstream _serviceIdStr;
            _serviceIdStr << hex << serviceId;
            serviceIdStr = _serviceIdStr.str();

            j["newService"] = {
                {"serviceId", serviceIdStr},
                {"ts", timestamp}
            };

            cout << j << endl;
        }

        virtual void onNewEnsemble(uint16_t eId) override
        {
            unsigned long int timestamp = time(NULL);
            json j;

            ensembleId = eId;
            stringstream _ensembleIdStr;
            _ensembleIdStr << hex << ensembleId;
            ensembleIdStr = _ensembleIdStr.str();

            j["newEnsemble"] = {
                {"ensembleId", ensembleIdStr},
                {"ts", timestamp}
            };

            cout << j << endl;
        }

        virtual void onSetEnsembleLabel(DabLabel& label) override
        {
            unsigned long int timestamp = time(NULL);
            ensembleLabel = label.utf8_label();
            json j;

            j["ensemble"] = {
                {"channel", channel},
                {"frequency", frequency},
                {"ensembleId", ensembleIdStr},
                {"ensembleLabel", ensembleLabel},
                {"ts", timestamp}
            };

            cout << j << endl;
        }

        // an ensemble could send a timestamp
        virtual void onDateTimeUpdate(const dab_date_time_t& dateTime) override
        {
            unsigned long int timestamp = time(NULL);
            json j;

            j["UTCTime"] = {
                {"year", dateTime.year},
                {"month", dateTime.month},
                {"day", dateTime.day},
                {"hour", dateTime.hour},
                {"minutes", dateTime.minutes},
                {"seconds", dateTime.seconds},
                {"ts", timestamp}
            };

            if (last_date_time != j) {
                cout << j << endl;
                last_date_time = j;
            }
        }

        virtual void onFIBDecodeSuccess(bool crcCheckOk, const uint8_t* fib) override
        {
        }

        virtual void onNewImpulseResponse(std::vector<float>&& data) override
        {
            (void)data;
        }

        virtual void onNewNullSymbol(std::vector<DSPCOMPLEX>&& data) override
        {
            (void)data;
        }

        virtual void onConstellationPoints(std::vector<DSPCOMPLEX>&& data) override
        {
            (void)data;
        }

        virtual void onMessage(message_level_t level, const std::string& text, const std::string& text2 = std::string()) override
        {
            std::string fullText;
            if (text2.empty())
                fullText = text;
            else
                fullText = text + text2;

            switch (level) {
                case message_level_t::Information:
                    cerr << "Info: " << fullText << endl;
                    break;
                case message_level_t::Error:
                    cerr << "Error: " << fullText << endl;
                    break;
            }
        }

        virtual void onTIIMeasurement(tii_measurement_t&& m) override
        {
            unsigned long int timestamp = time(NULL);
            json j;

            j["TII"] = {
                {"channel", channel},
                {"frequency", frequency},
                {"comb", m.comb},
                {"pattern", m.pattern},
                {"delay", m.delay_samples},
                {"delay_km", m.getDelayKm()},
                {"error", m.error},
                {"ts", timestamp}
            };

            cout << j << endl;
        }

        json last_snr;
        json last_date_time;
        bool synced = false;
        FILE* fic_fd = nullptr;

        int serviceId = 0;
        string serviceIdStr = "";
        string serviceLabel = "";

        int ensembleId = 0;
        string ensembleIdStr = "";
        string ensembleLabel = "";

        string channel = "";
        string frequency = "";

        string group;
        int portAudio;
        int portData;
};

struct service {
    string id;
    string mcastGroup;
    int mcastPort;
};

struct options_t {
    string soapySDRDriverArgs = "";
    string antenna = "";
    int gain = -1;
    string channel = "5A";
    vector<service> services;
    string frontend = "auto";
    string frontend_args = "";
    string dump_directory = ".";
    list<int> tests;

    RadioReceiverOptions rro;
};

options_t parse_cmdline(int argc, char **argv)
{
    options_t options;
    options.rro.decodeTII = false; // decode transmitter localisation ?
    vector<string> _services;
    vector<string> _service;

    int opt;
    while ((opt = getopt(argc, argv, "c:o:g:s:u")) != -1) {
        switch (opt) {
            case 'c':
                options.channel = optarg;
                break;
            case 'o':
                options.dump_directory = optarg;
                break;
            case 'g':
                options.gain = std::atoi(optarg);
                break;
            case 's':
                // format: SSSS:I.I.I.I,SSSS:I.I.I.I
                _services = split(optarg, ",");
                for (unsigned long i = 0; i < _services.size(); i++) {
                    _service = split(_services[i], ":");
                    // 0: serviceId
                    // 1: groupe multicast
                    struct service s = {
                        .id = _service[0],
                        .mcastGroup = _service[1]/*.c_str()*/,
                        .mcastPort = 1234,
                    };
                    std::transform(s.id.begin(), s.id.end(), s.id.begin(), [](unsigned char c) { return std::tolower(c); });
                    options.services.push_back(s);
                }
                break;
            case 'u':
                options.rro.disableCoarseCorrector = true;
                break;
            default:
                cerr << "Unknown option." << endl;
                exit(1);
        }
    }

    return options;
}

int main(int argc, char **argv)
{
    auto options = parse_cmdline(argc, argv);

    for (unsigned long i = 0; i < options.services.size(); i++) {
        std::cout << "id: " << options.services[i].id << ", mcastGroup: " << options.services[i].mcastGroup << ", mcastPort: " << options.services[i].mcastPort << std::endl;
    }

    RadioInterface ri;
    Channels channels;
    unique_ptr<CVirtualInput> in = nullptr;

    in.reset(CInputFactory::GetDevice(ri, options.frontend));

    if (not in) {
        cerr << "Could not start device" << endl;
        return 1;
    }

    if (options.gain == -1) {
        in->setAgc(true);
    } else {
        in->setGain(options.gain);
    }

#ifdef HAVE_SOAPYSDR
    if (not options.antenna.empty() and in->getID() == CDeviceID::SOAPYSDR) {
        dynamic_cast<CSoapySdr*>(in.get())->setDeviceParam(DeviceParam::SoapySDRAntenna, options.antenna);
    }

    if (not options.soapySDRDriverArgs.empty() and in->getID() == CDeviceID::SOAPYSDR) {
        dynamic_cast<CSoapySdr*>(in.get())->setDeviceParam(DeviceParam::SoapySDRDriverArgs, options.soapySDRDriverArgs);
    }
#endif

    auto freq = channels.getFrequency(options.channel);
    in->setFrequency(freq);

    ri.frequency = std::to_string(freq);
    ri.channel = options.channel;

    RadioReceiver rx(ri, *in, options.rro);

    rx.restart(false);

    cerr << "Waiting sync" << endl;
    while (not ri.synced) {
        this_thread::sleep_for(chrono::seconds(3));
    }

    cerr << "Waiting services list" << endl;
    while (rx.getServiceList().empty()) {
        this_thread::sleep_for(chrono::seconds(1));
    }

    // Wait an additional 3 seconds so that the receiver can complete the service list
    this_thread::sleep_for(chrono::seconds(3));

    using SId_t = uint32_t;
    map<SId_t, WavProgrammeHandler> phs;

    cerr << "Services found :" << endl;

    for (const auto& s : rx.getServiceList()) {
        cerr << "- [" << std::hex << s.serviceId << std::dec << "] " << s.serviceLabel.utf8_label();

        std::stringstream sstream;
        sstream << std::hex << s.serviceId;
        string service_id = sstream.str();
        // lowercase serviceIds
        if (service_id.begin() != service_id.end()) {
            auto it = service_id.begin();
            *it = std::tolower(*it);
        }

        // service filter
        bool bypass = true;
        unsigned int idx; // index du options.services à traiter
        for (idx = 0; idx < options.services.size(); idx++) {
            if (options.services[idx].id == service_id) {
                bypass = false;
                break;
            }
        }

        if (bypass) {
            cerr << " (BYPASS)" << endl;
            continue;
        } else {
            cerr << " (PROCESSING) idx " << idx << endl;
        }

        string dumpFilePrefix = options.dump_directory + "/" + sstream.str();
        mkdir(dumpFilePrefix.c_str(), 0755);
        dumpFilePrefix += "/" + sstream.str();
        dumpFilePrefix.erase(std::find_if(dumpFilePrefix.rbegin(), dumpFilePrefix.rend(),
                    [](int ch) { return !std::isspace(ch); }).base(), dumpFilePrefix.end());

        string filename_txt = dumpFilePrefix + ".ndjson";
        unsigned long int timestamp = time(NULL);

        json j;
        j["ensemble"] = {
            {"ensembleId", ri.ensembleIdStr},
            {"ensembleLabel", trim(ri.ensembleLabel)},
            {"channel", options.channel},
            {"frequency", freq},
            {"ts", timestamp}
        };
        cout << j << endl;
        ofstream file_txt;
        file_txt.open(filename_txt, std::ios_base::app);
        if (file_txt.is_open()) {
            cout << "write in " << filename_txt << endl;
            file_txt << j << endl;
            file_txt.close();
        } else {
            cerr << "ERROR: " << filename_txt << " unable to open" << endl;
        }

        for (const auto& sc : rx.getComponents(s)) {

            cerr << " [component "  << sc.componentNr <<
                " ASCTy: " <<
                (sc.audioType() == AudioServiceComponentType::DABPlus ? "DAB+" : "unknown") << " ]";

            const auto& sub = rx.getSubchannel(sc);
            cerr << " [subch " << sub.subChId << " bitrate:" << sub.bitrate() << " at SAd:" << sub.startAddr << "]";

            json j;
            j["service"] = {
                {"serviceId", s.serviceId}, // TODO hexa conversion
                {"serviceLabel", trim(s.serviceLabel.utf8_label())},
                {"componentNr", sc.componentNr},
                {"ascty", sc.audioType()},
                {"subCh", sub.subChId},
                {"bitrate", sub.bitrate()},
                {"startAddr", sub.startAddr},
                {"ts", timestamp}
            };
            cout << j << endl;
            file_txt.open(filename_txt, std::ios_base::app);
            if (file_txt.is_open()) {
                file_txt << j << endl;
                file_txt.close();
            } else {
                cerr << "ERROR: " << filename_txt << " unable to open" << endl;
            }
        }
        cerr << endl;

        cout << options.services[idx].mcastGroup << options.services[idx].mcastPort << endl;

        WavProgrammeHandler ph(s.serviceId, dumpFilePrefix, options.services[idx].mcastGroup.c_str(), options.services[idx].mcastPort);
        phs.emplace(std::make_pair(s.serviceId, std::move(ph)));

        auto dumpFileName = dumpFilePrefix + ".msc";

        if (rx.addServiceToDecode(phs.at(s.serviceId), dumpFileName, s) == false) {
            cerr << "Tune " << s.serviceId << " failed" << endl;
        }
    }

    // main loop
    while (true) {
        // don't let the main loop empty. Crash on MacOS...
        cout << "";
    }

    return 0;
}
