#include <cctype>
#include <dpp/dpp.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <json.hpp>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

const string PREFIX = "..";
dpp::embed embed = dpp::embed()
    .set_author("haruusyren", "https://innocence-studios.su/haruusyren/", "https://cdn.discordapp.com/avatars/755447668595097734/14f6ade12c465a62d159897d8779e026.webp?size=80")
    .set_color(dpp::colors::carnation_pink)
    .set_footer("Owned by Innocence Studios", "https://avatars.githubusercontent.com/u/153023970?v=4")
    .set_timestamp(time(0));


class TagManager {
public:
    map<string, vector<string>> data;
    map<string, vector<string>> parse() {
        ifstream tagFile("tags.data");
        string content((istreambuf_iterator<char>(tagFile)), istreambuf_iterator<char>());
        content.erase(remove_if(content.begin(), content.end(), [](char c) { return isspace(c) && c != ' '; }), content.end());
        size_t start = 0;
        parseJSON(content, start);
        tagFile.close();
        return data;
    };

    void writeTags() {
        ofstream tagFile;
        tagFile.open("tags.data");
        ostringstream oss;
        oss << "{\n";
        size_t i = 0;
        for (const auto& pair : data) {
            oss << "  \"" << pair.first << "\": [ ";
            for (size_t j = 0; j < pair.second.size(); j++) {
                oss << "\"" << pair.second[j] << "\"";
                if (j < pair.second.size() - 1) oss << ", ";
            }
            oss << " ]";
            if (i < data.size() - 1) oss << ",\n";
            i++;
        }
        oss << "\n}";
        tagFile << oss.str();
        tagFile.close();
    };

    string log() {
        string out;
        for (const auto& pair : data) {
            out += pair.first + ": ";
            for (const string& value : pair.second) {
                out += value + " ";
            }
            out += "\n";;
        };

        return out;
    };
private:
    void parseJSON(const string& json, size_t start) {
        while (start < json.length()) {
            size_t keyStart = json.find('"', start);
            size_t keyEnd = json.find('"', keyStart + 1);
            if (keyStart == string::npos || keyEnd == string::npos) return;

            size_t valueStart = json.find('[', keyEnd);
            if (valueStart == string::npos) return;

            size_t valueEnd = json.find(']', valueStart);
            if (valueEnd == string::npos) return;

            vector<string> values;
            parseValues(json, valueStart + 1, values);

            data[json.substr(keyStart + 1, keyEnd - keyStart - 1)] = values;

            start = valueEnd + 1;
        };
    };

    void parseValues(const string& json, size_t start, vector<string>& values) {
        while (start < json.length()) {
            if (isspace(json[start])) {
                start++;
                continue;
            };

            if (json[start] == '"') {
                size_t valueStart = start + 1;
                size_t valueEnd = json.find('"', valueStart);
                if (valueEnd == string::npos) return;
                values.push_back(json.substr(valueStart, valueEnd - valueStart));
                start = valueEnd + 1;
            }

            else if (json[start] == ',') start++;
            else if (json[start] == ']') return;
            else return;
        };
    };
};



static int rand(int min = 0, int max = RAND_MAX) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> distribution(min, max);

    return distribution(gen);
}

static int parseInt(string num) {
    try {
        return stoi(num);
    }
    catch (const invalid_argument& _) {
        return -RAND_MAX;
    };
}

template<typename T>
T findMap(map<string, T>& myMap, string key) {
    auto it = myMap.find(key);
    if (it != myMap.end()) return it->second;
    else return T();
};

template <typename T>
string joinVector(vector<T>& vec, char separator) {
    string out;

    for (T val : vec) {
        out += val + separator;
    };

    return out;
};

static vector<string> splitString(const string& input_string, char delimiter) {
    vector<string> tokens;
    stringstream ss(input_string);
    string token;

    while (getline(ss, token, delimiter)) {
        tokens.push_back(token);
    };

    return tokens;
}

static string command_tag_date(dpp::message_create_t event, string time) {
    return "<t:" + time + ">";
}
static string command_tag_random(dpp::message_create_t event, string range) {
    vector<string> minMax = splitString(range, '|');
    int min = parseInt(minMax[0]);
    int max = parseInt(minMax[1]);
    if (min == -RAND_MAX) return "";
    if (max == -RAND_MAX) return "";
    return to_string(rand(min, max));
};
static string command_tag_sum(dpp::message_create_t event, string range) {
    vector<string> nums = splitString(range, ',');
    int out = 0;
    for (string _num : nums) {
        int num = parseInt(_num);
        if (num == -RAND_MAX) return "";
        out += num;
    };
    return to_string(out);
};
static string command_tag_user(dpp::message_create_t event, string what) {
    cout << what << endl;
    if (what == "username") return event.msg.author.username;
    if (what == "name") return event.msg.author.global_name;
    if (what == "mention") return "<@" + to_string(event.msg.author.id) + ">";
    if (what == "id") return to_string(event.msg.author.id);
    return "";
}
static string command_tag_guild(dpp::message_create_t event, string what) {
    dpp::guild* g = dpp::find_guild(event.msg.guild_id);
    if (what == "name") return g->name;
    if (what == "created") return command_tag_date(event, to_string(static_cast<int>(g->get_creation_time())));
    if (what == "owner") return "<@" + to_string(g->owner_id) + ">";
    if (what == "id") return to_string(g->id);
    return "";
}
static string command_tag_created(dpp::message_create_t event, string which) {
    dpp::guild* g = dpp::find_guild(event.msg.guild_id);
    dpp::channel* c = dpp::find_channel(event.msg.channel_id);
    if (which == "user") return command_tag_date(event, to_string(static_cast<int>(event.msg.author.get_creation_time())));
    if (which == "channel") return command_tag_date(event, to_string(static_cast<int>(c->get_creation_time())));
    if (which == "guild" || which == "server") return command_tag_date(event, to_string(static_cast<int>(g->get_creation_time())));
    if (which == "message" || which == "now") return command_tag_date(event, to_string(static_cast<int>(event.msg.get_creation_time())));
    return "";
}
static string command_tag_format(vector<string> args, dpp::message_create_t event, map<string, function<string(dpp::message_create_t, string)>> tags) {
    string out;

    for (string arg : args) {
        if (arg.starts_with("{") && arg.ends_with("}")) {
            vector<string> keyValue = splitString(arg.substr(1, arg.length() - 2), ':');
            string key, value;
            if (keyValue.size() >= 1) key = keyValue[0];
            else key = "";
            if (keyValue.size() >= 2) value = keyValue[1];
            else value = "";
            if (key.empty()) key = value;
            function commandTag = findMap(tags, key);
            if (!commandTag) return "Tag `" + key + "` does not exist.";
            string result = commandTag(event, value);
            if (result.empty()) return "Tag `" + key + "` not valid.";
            out += result;
        }
        else out += arg;
        out += " ";
    };

    return out;
};
static dpp::message command_tag(dpp::message_create_t event, vector<string> args, map<string, vector<string>>& tags_) {
    map<string, function<string(dpp::message_create_t, string)>> tags = {
        { "user", command_tag_user },
        { "date", command_tag_date },
        { "sum", command_tag_sum },
        { "created", command_tag_created },
        { "random", command_tag_random },
        { "guild", command_tag_guild },
        { "server", command_tag_guild },
    };
    string action = args[0];
    args.erase(args.begin());
    string tagName;
    if (args.size() >= 1) {
        tagName = args[0];
        args.erase(args.begin());
    }
    else tagName = "";
    if (action == "create") {
        tags_[tagName] = args;
        dpp::embed embed_ = embed
            .set_title("Tag " + tagName + " created")
            .set_description("`" + joinVector(tags_[tagName], ' ') + "`")
            .add_field("Expected output", command_tag_format(args, event, tags));
        return dpp::message(event.msg.channel_id, embed_);
    };
    if (action == "info") {
        if (tagName.empty()) return dpp::message("Please specify a tag.");
        if (!tags_.contains(tagName)) return dpp::message("Tag not found.");
        dpp::embed embed_ = embed
            .set_title("Tag " + tagName)
            .set_description("`" + joinVector(tags_[tagName], ' ') + "`")
            .add_field("Expected output", command_tag_format(tags_[tagName], event, tags));
        return dpp::message(event.msg.channel_id, embed_);
    };
    if (action == "list") {
        dpp::embed embed_ = embed
        .set_title("Tag List")
            .set_description("List of all tags");
        for (auto pair : tags_) {
            embed_.add_field(pair.first, joinVector(pair.second, ' ') + " / " + command_tag_format(pair.second, event, tags));
        };
        return dpp::message(event.msg.channel_id, embed_);
    };

    tagName = action;
    if (tagName.empty()) return dpp::message("Please specify a tag.");
    if (!tags_.contains(tagName)) return dpp::message("Tag not found.");
    return dpp::message(command_tag_format(tags_[tagName], event, tags));

    return dpp::message("An error occured.");
}

map<string, function<dpp::message(dpp::message_create_t, vector<string>, map<string, vector<string>>&)>> COMMANDS = {
    { "tag", command_tag }
};


int main() {
    ifstream tokenFile("TOKEN");
    const string TOKEN((istreambuf_iterator<char>(tokenFile)), istreambuf_iterator<char>());
    cout << "---------- STARTING ----------" << endl;
    TagManager manager;
    manager.parse();
    cout << manager.log() << endl;

    dpp::cluster bot(TOKEN, dpp::i_default_intents | dpp::i_message_content);

    cout << "----- Client starting... -----" << endl;

    // bot.on_log(dpp::utility::cout_logger());

    /*bot.on_slashcommand([](const dpp::slashcommand_t& interaction) {
        if (interaction.command.get_command_name() == "ping") {
            interaction.reply("Pong! and no");
        }

        if (interaction.command.get_command_name() == "tag") {
            interaction.reply("no");
            cout << interaction.from->creator << endl;
            string content = get<string>(interaction.get_parameter("content"));
        }
    });*/

    bot.on_message_create([&bot, &manager](const dpp::message_create_t& event) {
        if (!event.msg.content.starts_with(PREFIX)) return;

        string content = event.msg.content;
        content.erase(0, PREFIX.length());
        istringstream iss(content);
        string command;
        iss >> command;

        function commandFunction = findMap(COMMANDS, command);
        if (!commandFunction) return;
        vector<string> args = splitString(event.msg.content, ' ');
        args.erase(args.begin());
        dpp::message message = commandFunction(event, args, manager.data);

        event.reply(message, false);
        manager.writeTags();
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        /*if (dpp::run_once<struct register_bot_commands>()) {
            vector<dpp::slashcommand> commands;
            bot.global_command_create(dpp::slashcommand("ping", "Ping pong!", bot.me.id));

            dpp::slashcommand commandTag = dpp::slashcommand("tag", "Creates a tag", bot.me.id);
            commandTag.add_option(
                dpp::command_option(dpp::co_string, "content", "Tag content", true)
            );
            bot.global_command_create(dpp::slashcommand("tag", "Creates a tag", bot.me.id));
            
            for (dpp::slashcommand command : commands) {
                // bot.global_command_create(command);
                cout << "  Command " << command.name << " created." << endl;
            };
        };*/
        bot.set_presence(dpp::presence(dpp::presence_status::ps_idle, dpp::activity(dpp::activity_type::at_streaming, "name", "some C++", "https://twitch.tv/syren_off/")));
        cout << "------- Client started. ------" << endl;
    });

    bot.start(dpp::st_wait);

    return 0x45;
}
