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
#include "backend/radio-receiver.h"
#include "input/input_factory.h"
#include "various/channels.h"
#include "libs/json.hpp"

using namespace std;
using namespace nlohmann;

const std::string WHITESPACE = " \n\r\t\f\v";

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

vector<string> split (string s, string delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

class WavProgrammeHandler: public ProgrammeHandlerInterface
{
    public:
        WavProgrammeHandler(uint32_t SId, const std::string& fileprefix) :
            SId(SId),
            filePrefix(fileprefix) {}

        WavProgrammeHandler(const WavProgrammeHandler& other) = delete;
        WavProgrammeHandler& operator=(const WavProgrammeHandler& other) = delete;
        WavProgrammeHandler(WavProgrammeHandler&& other) = default;
        WavProgrammeHandler& operator=(WavProgrammeHandler&& other) = default;

        virtual void onFrameErrors(int frameErrors) override
        {
            (void)frameErrors;
        }

        virtual void onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, const string& mode) override
        {
            // un nouveau buffer audio pcm a été reçu, on l'écrit dans le fichier

            string filename = filePrefix + ".pcm";
            FILE *file = fopen(filename.c_str(),"ab");
            fwrite(audioData.data(),sizeof(short),audioData.size(),file);
            fclose(file);
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
                {"ts", timestamp}
            };
            file << j << endl;

            file.close();

            cout << "[" << std::hex << SId << std::dec << "] " << j << endl;
        }

        virtual void onMOT(const mot_file_t& mot_file) override
        {
            string extension;
            if (mot_file.content_sub_type == 0x01) {
                extension = "jpg";
            } else if (mot_file.content_sub_type == 0x03) {
                extension = "png";
            }

            ofstream file_mot, file_txt;
            unsigned long int timestamp = time(NULL);

            uint32_t current_mot_size = mot_file.data.size();
            if (current_mot_size == last_size) {
                // détection de doublon par la taille du fichier
                cout << "[" << std::hex << SId << std::dec << "] MOT BYPASS (doublon " << last_size << " octets)" << endl;
                return;
            }
            last_size = current_mot_size;

            string filename_mot = filePrefix + "-" + std::to_string(timestamp) + "." + extension;
            string filename_txt = filePrefix + ".ndjson";
            file_txt.open(filename_txt, std::ios_base::app);

            json j;
            j["mot"] = {
                {"file", filename_mot.substr(filename_mot.find_last_of("/\\") + 1)},
                {"content_name", mot_file.content_name},
                {"click_through_url", mot_file.click_through_url},
                {"category_title", mot_file.category_title},
                {"ts", timestamp}
            };

            file_txt << j << endl;
            file_txt.close();

            // enregistrement de l'image MOT
            file_mot.open(filename_mot);
            std::stringstream ss;
            for (auto it = mot_file.data.begin(); it != mot_file.data.end(); it++) {
                ss << *it;
            }
            file_mot << ss.str();
            file_mot.close();

            cout << "[" << std::hex << SId << std::dec << "] " << j << endl;
        }

        virtual void onPADLengthError(size_t announced_xpad_len, size_t xpad_len) override
        {
            cout << "X-PAD length mismatch, expected: " << announced_xpad_len << " got: " << xpad_len << endl;
        }

    private:
        uint32_t last_size = 0; // stocke la taille en octets du dernier MOT
        uint32_t SId;
        string filePrefix;
};


class RadioInterface : public RadioControllerInterface
{
    public:
        // rapport signal sur bruit
        virtual void onSNR(float snr) override
        {
            unsigned long int timestamp = time(NULL);
            json j;

            j["snr"] = {
                {"ts", timestamp},
                {"value", snr},
                {"channel", channel},
                {"frequency", frequency}
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
            serviceId = sId;
            stringstream _serviceIdStr;
	    _serviceIdStr << hex << serviceId;
	    serviceIdStr = _serviceIdStr.str();

            cout << "Nouveau service détecté: serviceId=" << _serviceIdStr.str() << endl;
        }

        virtual void onNewEnsemble(uint16_t eId) override
        {
            ensembleId = eId;
	    stringstream _ensembleIdStr;
	    _ensembleIdStr << hex << ensembleId;
            ensembleIdStr = _ensembleIdStr.str();

            cout << "Nouvel ensemble: ensembleId=" << _ensembleIdStr.str() << endl;
        }

        virtual void onSetEnsembleLabel(DabLabel& label) override
        {
            ensembleLabel = label.utf8_label();

	    json j;
	    j["ensemble"] = {
                {"channel", channel},
		{"frequency", frequency},
                {"ensembleId", ensembleIdStr},
                {"ensembleLabel", ensembleLabel}
            };
	    cout << j << endl;
        }

        // le multiplex indique l'heure
        virtual void onDateTimeUpdate(const dab_date_time_t& dateTime) override
        {
            json j;
            j["UTCTime"] = {
                {"year", dateTime.year},
                {"month", dateTime.month},
                {"day", dateTime.day},
                {"hour", dateTime.hour},
                {"minutes", dateTime.minutes},
                {"seconds", dateTime.seconds}
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
            json j;
            j["TII"] = {
                {"channel", channel},
                {"frequency", frequency},
                {"comb", m.comb},
                {"pattern", m.pattern},
                {"delay", m.delay_samples},
                {"delay_km", m.getDelayKm()},
                {"error", m.error}
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
};

struct options_t {
    int gain = -1;
    string channel = "5A";
    vector<string> services;
    string frontend = "auto";
    string frontend_args = "";
    string dump_directory = "";
    list<int> tests;

    RadioReceiverOptions rro;
};

options_t parse_cmdline(int argc, char **argv)
{
    options_t options;
    options.rro.decodeTII = false; // décodage de localisation de l'émetteur ?

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
                options.services = split(optarg, ",");
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

    auto freq = channels.getFrequency(options.channel);
    in->setFrequency(freq);
    string service_to_tune = "";

    ri.frequency = std::to_string(freq);
    ri.channel = options.channel;

    RadioReceiver rx(ri, *in, options.rro);

    rx.restart(false);

    cerr << "En attente de synchronisation" << endl;
    while (not ri.synced) {
        this_thread::sleep_for(chrono::seconds(3));
    }

    cerr << "En attente de la liste des services" << endl;
    while (rx.getServiceList().empty()) {
        this_thread::sleep_for(chrono::seconds(1));
    }

    // Wait an additional 3 seconds so that the receiver can complete the service list
    this_thread::sleep_for(chrono::seconds(3));

    using SId_t = uint32_t;
    map<SId_t, WavProgrammeHandler> phs;

    cerr << "Liste des services trouvés :" << endl;
    // boucle des services
    for (const auto& s : rx.getServiceList()) {
        cerr << "- [" << std::hex << s.serviceId << std::dec << "] " << s.serviceLabel.utf8_label() << endl;

        std::stringstream sstream;
        sstream << std::hex << s.serviceId;
        string service_id = sstream.str();
        // conversion en minuscules
        if (service_id.begin() != service_id.end()) {
            auto it = service_id.begin();
            *it = std::tolower(*it);
        }

        // si filtrage par service
        if (!options.services.empty() && std::find(options.services.begin(), options.services.end(), service_id) == options.services.end()) {
            cerr << " BYPASS" << endl;
            continue;
        }

        string dumpFilePrefix = options.dump_directory + "/" + sstream.str();
        mkdir(dumpFilePrefix.c_str(), 0755);
        dumpFilePrefix += "/" + sstream.str();
        dumpFilePrefix.erase(std::find_if(dumpFilePrefix.rbegin(), dumpFilePrefix.rend(),
                    [](int ch) { return !std::isspace(ch); }).base(), dumpFilePrefix.end());

        ofstream file_txt;
        string filename_sid = dumpFilePrefix + ".ndjson";
        file_txt.open(filename_sid, std::ios_base::app);
        unsigned long int timestamp = time(NULL);

        json je;
        je["ensemble"] = {
            {"emsembleId", ri.ensembleIdStr},
            {"ensembleLabel", trim(ri.ensembleLabel)},
            {"channel", options.channel},
            {"frequency", freq},
            {"ts", timestamp}
        };
        file_txt << je << endl;

        // boucle des composants
        for (const auto& sc : rx.getComponents(s)) {

            cerr << " [component "  << sc.componentNr <<
                " ASCTy: " <<
                (sc.audioType() == AudioServiceComponentType::DABPlus ? "DAB+" : "unknown") << " ]";

            const auto& sub = rx.getSubchannel(sc);
            cerr << " [subch " << sub.subChId << " bitrate:" << sub.bitrate() << " at SAd:" << sub.startAddr << "]";

            json js;
            js["service"] = {
                {"serviceId", s.serviceId},
                {"serviceLabel", trim(s.serviceLabel.utf8_label())},
                {"componentNr", sc.componentNr},
                {"ascty", sc.audioType()},
                {"subCh", sub.subChId},
                {"bitrate", sub.bitrate()},
                {"startAddr", sub.startAddr},
                {"ts", timestamp}
            };
            file_txt << js << endl;

        }
        cerr << endl;

        file_txt.close();

        WavProgrammeHandler ph(s.serviceId, dumpFilePrefix);
        phs.emplace(std::make_pair(s.serviceId, move(ph)));

        auto dumpFileName = dumpFilePrefix + ".msc";

        if (rx.addServiceToDecode(phs.at(s.serviceId), dumpFileName, s) == false) {
            cerr << "Tune to " << service_to_tune << " failed" << endl;
        }
    }

    while (true) {
        cerr << "**** Enter '.' to quit." << endl;
        cin >> service_to_tune;
        if (service_to_tune == ".") {
            break;
        }
    }

    return 0;
}
