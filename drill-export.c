#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include <strsafe.h>
#include <tchar.h>

/**
 @brief Inserts substring into an existing char array which needs to be big enough to fits the additional substring
 @param str The string that gets modified
 @param ins The substring which gets inserted
 @param index The index of the string at which the substring gets inserted
 */
void str_insert(char* str, char* ins, unsigned long index) {
    unsigned long str_len = strlen(str);
    unsigned long ins_len = strlen(ins);
    int offset = 0;
    
    for (unsigned long i = str_len; i >= index && offset <= str_len; i--)
        str[str_len + ins_len - offset++] = str[i];
    for (unsigned long i = 0; i < ins_len; i++)
        str[i+index] = ins[i];
}

/**
 @brief Deletes a part of a string
 @param str The string which gets modified
 @param index The starting index of the substring which gets removed
 @param length The number of characters which get removed, starting from index
 */
void str_remove(char* str, size_t index, size_t length) {
    size_t str_len = strlen(str);
    
    for (size_t i = index; i <= str_len; i++) {
        str[i] = str[i + length];
    }
}

/**
 @brief Checks if a string contains a given substring
 @param str The string to search through
 @param substr The string after which shall be looked
 @return The beginning index of the found substring. Otherwise -1
 */
int str_contains(char *str, char *substr) {
    size_t str_len = strlen(str);
    size_t substr_len = strlen(substr);

    if (substr_len > str_len) return -1;
    if (substr_len < 1) return -1;
    
    size_t found_index;
    for (size_t i = 0; i <= str_len - substr_len; i++) {
        size_t j;
        for (j = 0, found_index = i; j < substr_len; j++) {
            if (str[i+j] != substr[j]) break;
        }
        
        if (j == substr_len) return (int)found_index;
    }
    return -1;
}


/**
 @brief Replaced the first occurance of a given substring
 @param str The string which gets modified
 @param from The substring which gets replaced
 @param to The substring which takes the place of from
 @return 0 if successful, otherweise -1
 */
int str_replace_first(char* str, char* from, char* to) {
    int from_pos = str_contains(str, from);
    if (from_pos < 0) return -1;
    
    unsigned long from_len = strlen(from);
    unsigned long to_len = strlen(to);
    
    if (from_len == to_len) {
        for (int i = 0; i < from_len; i++)
            str[from_pos+i] = to[i];
        return 0;
    }
    
    str_remove(str, from_pos, strlen(from));
    str_insert(str, to, from_pos);
    
    return 0;
}

/**
 * @brief A more robust way of running shell commands than system()
*/
void run_command(char *cmd) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    CreateProcess(NULL, // No module name (use command line)
        cmd,            // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi);           // Pointer to PROCESS_INFORMATION structure

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void remove_quotation_marks(char* str) {
    size_t len = strnlen_s(str, MAX_PATH);
    if (len < 1) return;

    if (str[0] == '\"' && str[len-1] == '\"') {
        int i;
        for (i = 0; i < len-1; i++) {
            str[i] = str[i+1];
        }
        str[i-1] = '\0';
    }
}

void remove_filename_from_path(char* path) {
    size_t len = strlen(path);
    
    if (len < 1) return;
    if (path[len-1] == '\\') return;
    
    while (len > 0) {
        if (path[--len] == '\\') {
            path[len+1] = '\0';
            break;
        }
    }
}

/**
 * @brief takes the filename from a full path and appends it to another path
 * @arg mutable path
 * @arg unmutable path with filename
*/
void cat_filename(char* path, const char* filename) {
    size_t path_len = strnlen_s(path, MAX_PATH);
    size_t filename_len = strnlen_s(filename, MAX_PATH);
    size_t start_index;

    for (start_index = filename_len; start_index > 0; start_index--) {
        if (filename[start_index-1] == '\\') break;
    }

    for (size_t i = 0; i < filename_len; i++) {
        path[path_len+i] = filename[start_index + i];
    }
}

/**
 * @arg Optional path for kicad_pcb file. If none given, the current directory is searched for kicad_pcb files.
 * If nothing is to be found, the user keeps getting prompted for a file path.
*/
int main(int argc, char *argv[]) {

    unsigned int pcb_files_found = 0;
    char pcb_file_path[MAX_PATH];
    
    if (argc > 1) {
        // check file availability
        if(fopen(argv[1], "r")) {
            pcb_files_found++;
            strcpy(pcb_file_path, argv[1]);
        }
    } else {
        // search current directory
        WIN32_FIND_DATA ffd;
        TCHAR szDir[MAX_PATH];
        HANDLE hFind = INVALID_HANDLE_VALUE;
        StringCchCopy(szDir, MAX_PATH, _T(".\\*.kicad_pcb"));

        hFind = FindFirstFile(szDir, &ffd);
        char pcb_file_paths[MAX_PATH][MAX_PATH];
        do {
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strnlen_s(ffd.cFileName, MAX_PATH) > 10) {
                strcpy(pcb_file_paths[pcb_files_found++],ffd.cFileName);
                printf("%d) %s\n", pcb_files_found, ffd.cFileName);
            }
        } while (FindNextFile(hFind, &ffd) != 0);
        FindClose(hFind);

        if (pcb_files_found == 1) {
            strcpy(pcb_file_path, pcb_file_paths[0]);
        }
        else if (pcb_files_found > 1) {
            // print multiple files found at that directory, which one to take
            char input[MAX_PATH];
            int selection;
            do {
                int isnumber;
                do {
                    isnumber = 1;
                    printf("\nQuelldatei: ");
                    gets_s(input, MAX_PATH);
                    for (int i = 0; i < strnlen_s(input, MAX_PATH); i++) {
                        if (!isdigit(input[i])) isnumber = 0;
                    }
                } while(!isnumber);
                selection = atoi(input);
            } while(selection > pcb_files_found || selection <= 0);
            strcpy(pcb_file_path, pcb_file_paths[selection-1]);
        }
    }
    
    while (pcb_files_found < 1) {
        printf("Pfad der kicad_pcb-Datei: ");
        gets_s(pcb_file_path, MAX_PATH);
        remove_quotation_marks(pcb_file_path);
        if(fopen(pcb_file_path, "r")) pcb_files_found++;
    }



    // find kicad-cli
    char *kicad_cli = malloc(MAX_PATH);
    strcpy(kicad_cli, "C:\\Program Files\\KiCad\\8.0\\bin\\kicad-cli.exe");

    if (!fopen(kicad_cli, "r")) {
        strcpy(kicad_cli, "C:\\Program Files\\KiCad\\7.0\\bin\\kicad-cli.exe");
    }

    while (!fopen(kicad_cli, "r")) {
        printf("Dateipfad von kicad-cli.exe: ");
        gets_s(kicad_cli, MAX_PATH);
        remove_quotation_marks(kicad_cli);
    }

    char *kicad_cli_promt = "\" pcb export drill --drill-origin plot --excellon-zeros-format suppressleading -u in --excellon-min-header \"";

    char *command = malloc(strlen(kicad_cli) + strlen(kicad_cli_promt) + strlen(pcb_file_path) + 1);
    // "quotes" removed for fopen to work and added back for system

    strcpy(command, "\"");
    strcat(command, kicad_cli);
    strcat(command, kicad_cli_promt);
    strcat(command, pcb_file_path);
    strcat(command, "\"");

    run_command(command);
    free(command);
    free(kicad_cli);

    char *drl_file_path; // export from KiCad
    char *exc_file_path; // to be exported as EAGLE-compatible
    // both are the same file path as pcb_file_path, with different endings

    int i;
    drl_file_path = malloc(strlen(pcb_file_path)-6);
    for (i = 0; i < strlen(pcb_file_path)-9; i++) {
        drl_file_path[i] = pcb_file_path[i];
    }
    drl_file_path[i] = '\0';
    strcat(drl_file_path, "drl");

    exc_file_path = malloc(strlen(pcb_file_path)-6);
    for (i = 0; i < strlen(pcb_file_path)-9; i++) {
        exc_file_path[i] = pcb_file_path[i];
    }
    exc_file_path[i] = '\0';
    strcat(exc_file_path, "exc");

    // the drl file always gets exported where the program is started
    // if the pcb file is somewhere else, the drl file must be moved
    char exe_path[MAX_PATH];
    GetModuleFileName(NULL, exe_path, MAX_PATH);
    remove_filename_from_path(exe_path);

    cat_filename(exe_path, drl_file_path);

    printf("moving %s to %s", exe_path, drl_file_path);

    MoveFile(exe_path, drl_file_path);

    FILE* drl_file = fopen(drl_file_path, "r");
    FILE* exc_file = fopen(exc_file_path, "w");

    char line[MAX_PATH];
    // skip the tool definitions
    while(fgets(line, MAX_PATH, drl_file) && strcmp(line, "T1\n") != 0);

    // copy contents from drl to exc file with absolute X coordinates
    do {
        str_replace_first(line, "X-", "X");
        fputs(line, exc_file);
    } while(fgets(line, MAX_PATH, drl_file));
    
    fclose(drl_file);
    remove(drl_file_path);
    fclose(exc_file);

    free(drl_file_path);
    free(exc_file_path);
    return 0;
}