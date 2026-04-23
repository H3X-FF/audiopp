#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <iomanip>

#include <ncursesw/ncurses.h>

#include "file_commands.hpp"
#include "command_mode_and_pipe.hpp"
#include "states.hpp"
#include "sort_commands.hpp"

#define ESCAPE_KEY 27
#define ENTER_KEY '\n'
#define BACKSPACE_8 8
#define BACKSPACE_127 127

void CommandPipe::validate::validateCommand(const std::vector<std::string>& tokens, AppState& appState) {
    // Registry of available commands and their requirements
    static std::unordered_map<std::string, CommandProperties> commandRegistry{

        {"scan", {1, {"--recurse"}, scan}},
        {"rm", {1, {}, rm}},
        {"rename", {2, {}, rname}},
        {"sort", {1, {"--normal", "--reverse"}, sortList}}

    };

    std::string command{tokens[0]};

    // Validate command existence
    if (commandRegistry.count(command) == 0) {
        printError("Invalid Command: " + command);

        return;
    }

    std::vector<std::string> args, flags;

    // Distinguish between positional arguments and optional flags
    for (int i{1}; i < tokens.size(); i++) {
        if (args.size() < commandRegistry[command].minArgs) {
            args.push_back(tokens[i]);
        }
        else if (std::find(commandRegistry[command].allowedFlags.begin(), commandRegistry[command].allowedFlags.end(),
            tokens[i]) != commandRegistry[command].allowedFlags.end()) {
            flags.push_back(tokens[i]);
        }
        else {
            printError("Invalid arguments/flags: " + tokens[i]);
            return;
        }
    }

    if (args.size() < commandRegistry[command].minArgs) {
        printError("Not enough arguments");
        return;
    }

    commandRegistry[command].action(args, flags, appState);
}

void CommandPipe::setUpCommand(std::string prompt, AppState& appState) {
    if (prompt.size() < 1) return;

    std::string currToken;
    std::stringstream stream(prompt);
    std::vector<std::string> tokens;

    while (stream >> std::quoted(currToken, '\'')) {
        tokens.push_back(currToken);
    }

    validate::validateCommand(tokens, appState);
}



// The input field
void commandMode(AppState& appState) {
    char command[512];

    curs_set(1);
    bkgdset(A_REVERSE);
    move(LINES-1, 0);
    clrtoeol();
    addch(':');

    appState.inCommandMode = true;

    bool canceled = false;
    int promptLen = 0;
    int cursorPos = 0;
    int viewOffset = 0;
    int maxVisible = COLS-2;

    while (promptLen < sizeof(command)-1) {
        int cmdCh = getch();


        // --- SCROLLING ---
        if (cursorPos >= viewOffset + maxVisible) {
            viewOffset = cursorPos - maxVisible + 1;
        }
        else if (cursorPos < viewOffset) {
            viewOffset = cursorPos;
        }

        move(LINES-1, 0);
        clrtoeol();
        addch(':');

        if (viewOffset < promptLen) {
            printw("%.*s", maxVisible, &command[viewOffset]);
        }

        move(LINES-1, (cursorPos - viewOffset) + 1);
        refresh();

        // --- CONTROLS ---
        if (cmdCh == ESCAPE_KEY) {
            canceled = true;
            break;
        }
        if (cmdCh == ENTER_KEY) break;

        if (cmdCh == KEY_LEFT) {
            if (cursorPos > 0) {
                cursorPos--;
                move(LINES-1, cursorPos+1);
            }
        }
        else if (cmdCh == KEY_RIGHT) {
            if (cursorPos < promptLen) {
                cursorPos++;
                move(LINES-1, cursorPos+1);
            }
        }
        else if (cmdCh == KEY_BACKSPACE || cmdCh == BACKSPACE_8 || cmdCh == BACKSPACE_127 || cmdCh == KEY_DC) {
            if (cursorPos > 0) {

                for (int i = cursorPos - 1; i < promptLen; i++) {
                    command[i] = command[i + 1];
                }

                promptLen--;
                cursorPos--;
                command[promptLen] = '\0';

                move(LINES-1, cursorPos + 1);

                printw("%s ", &command[cursorPos]);

                move(LINES-1, cursorPos + 1);
            }
        }
        else if (isprint(cmdCh) && promptLen < sizeof(command) - 1) {

            for (int i = promptLen; i > cursorPos; i--) {
                command[i] = command[i - 1];
            }
            command[cursorPos] = cmdCh;
            promptLen++;
            cursorPos++;
            command[promptLen] = '\0';

            move(LINES-1, cursorPos);
            printw("%s", &command[cursorPos-1]);
            move(LINES-1, cursorPos + 1);
        }
    }

    curs_set(0);
    move(LINES-1, 0);
    bkgdset(A_NORMAL);
    clrtoeol();

    if (!canceled) {
        command[promptLen] = '\0';
        CommandPipe::setUpCommand(command, appState);
    }

    command[0] = '\0';

    appState.inCommandMode = false;
    appState.shouldRedraw = true;
}