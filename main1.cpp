#include <bits/stdc++.h>
#include <stdio.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#define MAX_CHAR 4096
#define BUF_SIZE 65536
using namespace std;

char root[MAX_CHAR];
char curr_dir[MAX_CHAR];
vector<string> d_nameList;
int cur_window = 0;
unsigned int term_row;
unsigned int term_col;
struct winsize terminal;
struct termios orig_termios, new_termios;
int cursor_x = 1, cursor_y = 1;
int sflag = 0;
stack<string> past_path;
stack<string> forw_path;
vector<string> command_tokens;
string source_path, dest_path;
// bool is_goto_called=false;

void listDir(const char *root);
void display(const char *fileName);
void onNCanonicalMode();
void moveUp();
void moveDown();
void moveRight();
void moveLeft();
void moveBack();
void enter();
void home();
int enterCommandMode();
void modify_dlist();
void clearStack();
void printCursor();
void splitCommand(string);
int copy();
void move();
void create_file();
void create_dir();
void delete_file();
void delete_dir();
void goto_path();
void search();
void renameFunc();
void copyDirectory(string, string);
void copyFile(string, string);
void recursiveDelete(string);

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        getcwd(root, sizeof(root));
    }
    else if (argc == 2)
    {
        strcpy(root, argv[1]);
    }
    else
    {
        cout << "too many args\n";
        exit(0);
    }
    strcat(curr_dir, root);
    past_path.push(root);
    printf("%c[?1049h", 27); //Alternate buffer on
    listDir(root);
    onNCanonicalMode();
    return 0;
}

void listDir(const char *path)
{
    struct dirent *de; // Pointer for directory entry

    // opendir() returns a pointer of DIR type.
    DIR *dr = opendir(path);

    if (dr == NULL) // opendir returns NULL if couldn't open directory
    {
        printf("Could not open current directory");
        return;
    }

    printf("\033[H\033[J");
    printf("%c[%d;%dH", 27, 1, 1);
    d_nameList.clear();
    // int last_row = terminal.ws_row;
    // printf("%c[%d;%dH", 27, last_row, 1);
    // printf("In Normal Mode");
    while ((de = readdir(dr)) != NULL)
    {
        if (strcmp(path, root) == 0)
        {
            strcpy(curr_dir, root);
            if (strcmp(de->d_name, "..") == 0)
                continue;
        }
        d_nameList.push_back(de->d_name);
    }

    sort(d_nameList.begin(), d_nameList.end());
    modify_dlist();

    int last_row = terminal.ws_row;
    printf("%c[%d;%dH", 27, last_row, 1);
    printf("In Normal Mode");
    printf("%c[%d;%dH", 27, 1, 1);
    // printf("\x1b[31m\x1b[44m");
    closedir(dr);
}

void modify_dlist()
{
    // int last_row = terminal.ws_row;
    // printf("%c[%d;%dH", 27, last_row, 1);
    // printf("In Normal Mode");
    write(STDOUT_FILENO, "\x1b[2J", 4); //to clear screen
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal);
    term_row = terminal.ws_row - 2;
    term_col = terminal.ws_col;
    int numFiles = d_nameList.size();
    int from = cur_window;
    int to;
    if (numFiles <= term_row)
        to = numFiles - 1;
    else
        to = term_row + cur_window;
    for (int i = from; i <= to; i++)
    {
        string fileName = d_nameList[i];
        display(fileName.c_str());
    }
    return;
}
/*============================================================
takes path and check if directory or not.
=============================================================*/
bool isDirectory(string path)
{
    struct stat sb;
    if (stat(path.c_str(), &sb) != 0)
    {
        perror(path.c_str());
        return false;
    }
    if (S_ISDIR(sb.st_mode))
        return true;
    else
        return false;
}
/*============================================================
take relative path and convert to absolute path for internal
system calls.
=============================================================*/
string get_absolute_path(string r_path)
{
    string abs_path = "";
    if (r_path[0] == '~')
    {
        r_path = r_path.substr(1, r_path.length());
        abs_path = string(root) + r_path;
    }
    else if (r_path[0] == '/')
    {
        abs_path = string(root) + r_path;
    }
    else if (r_path[0] == '.' && r_path[1] == '/')
    {
        abs_path = string(curr_dir) + r_path.substr(1, r_path.length());
    }
    else
    {
        abs_path = string(curr_dir) + "/" + r_path;
    }
    return abs_path;
}

void display(const char *fileName)
{
    struct stat sb;
    string filePath = get_absolute_path(fileName);
    stat(filePath.c_str(), &sb);

    printf((S_ISDIR(sb.st_mode)) ? "d" : "-");
    printf((sb.st_mode & S_IRUSR) ? "r" : "-");
    printf((sb.st_mode & S_IWUSR) ? "w" : "-");
    printf((sb.st_mode & S_IXUSR) ? "x" : "-");
    printf((sb.st_mode & S_IRGRP) ? "r" : "-");
    printf((sb.st_mode & S_IWGRP) ? "w" : "-");
    printf((sb.st_mode & S_IXGRP) ? "x" : "-");
    printf((sb.st_mode & S_IROTH) ? "r" : "-");
    printf((sb.st_mode & S_IWOTH) ? "w" : "-");
    printf((sb.st_mode & S_IXOTH) ? "x" : "-");

    long long x = sb.st_size;
    printf("%10lld ", x);
    string m_time = string(ctime(&sb.st_mtime));
    m_time = m_time.substr(4, 12);
    printf(" %-12s ", m_time.c_str());

    if (S_ISDIR(sb.st_mode))
    {
        printf("\033[1;32m");
        printf("\t%-20s\n", fileName);
        printf("\033[0m");
    }
    else
        printf("\t%-20s\n", fileName);
}

void onNCanonicalMode()
{
    int last_row = terminal.ws_row;
    printf("%c[%d;%dH", 27, last_row, 1);
    printf("In Normal Mode");
    cursor_x = cursor_y = 1;
    printCursor();
    tcgetattr(STDIN_FILENO, &orig_termios);
    new_termios = orig_termios;

    new_termios.c_lflag &= ~(ICANON | ECHO | IEXTEN | ISIG);
    new_termios.c_iflag &= ~(BRKINT);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios);

    // char input[3] = {0};
    char ch;
    while (1)
    {
        ch = cin.get();
        // cout << ch<<endl;
        if (ch == 27)
        {
            while (1)
            {
                ch = cin.get();
                ch = cin.get();

                if (ch == 'A')
                {
                    moveUp();
                    break;
                }
                else if (ch == 'B')
                {
                    moveDown();
                    break;
                }
                else if (ch == 'C')
                {
                    moveRight();
                    break;
                }
                else if (ch == 'D')
                {
                    moveLeft();
                    break;
                }
                else
                {
                    break;
                }
            }
        }
        else if (ch == 'h' || ch == 'H')
        {
            home();
        }
        else if (ch == 127)
            moveBack();
        else if (ch == 10)
            enter();
        else if (ch == ':')
        {
            cout << "helo  : \n";
            int ret_value = enterCommandMode();

            cout << "exit command mode\n";
            listDir(curr_dir);
        }
        else if (ch == 'q')
        {
            // cout << "\nexi/t()";
            printf("\033[H\033[J");
            printf("%c[%d;%dH", 27, 1, 1);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            exit(0);
        }
    }
}

//**********************************************************************
// This Method Clear the stack contents
//**********************************************************************
void clearStack(stack<string> &s)
{
    while (!s.empty())
    {
        s.pop();
    }
}

void printCursor()
{
    printf("%c[%d;%dH", 27, cursor_x, cursor_y);
}

void moveUp()
{
    if (cursor_x > 1)
    {
        cursor_x--;
        printCursor();
    }
    else if (cursor_x == 1 && cursor_x + cur_window > 1)
    {
        cur_window--;
        //listdir(cur_dir);
        modify_dlist();
        printCursor();
    }
}

void moveDown()
{
    if (cursor_x <= term_row && cursor_x < d_nameList.size())
    {
        cursor_x++;
        printCursor();
    }
    else if (cursor_x > term_row && cursor_x + cur_window < d_nameList.size())
    {
        cur_window++;
        //listdir(cur_dir);
        modify_dlist();
        printCursor();
    }
}

void moveRight()
{
    if (!forw_path.empty())
    {
        string cpath = string(curr_dir);
        if (sflag != 1)
            past_path.push(string(curr_dir));
        string top = forw_path.top();
        forw_path.pop();
        strcpy(curr_dir, top.c_str());
        //cout<<"******* RIGHT: "<<curPath;
        sflag = 0;
        listDir(curr_dir);
        cursor_x = 1, cursor_y = 1;
        printCursor();
    }
    // cout << "inq C\n";
}

void moveLeft()
{
    if (!past_path.empty())
    {
        string cpath = string(curr_dir);
        if (sflag != 1)
            forw_path.push(string(curr_dir));
        string top = past_path.top();
        past_path.pop();
        strcpy(curr_dir, top.c_str());
        //cout<<"******* RIGHT: "<<curPath;
        sflag = 0;
        listDir(curr_dir);
        cursor_x = 1, cursor_y = 1;
        printCursor();
    }
    // cout << "in D\n";
}

void moveBack()
{
    string cpath = string(curr_dir);
    if ((strcmp(curr_dir, root) != 0) && sflag != 1)
    {
        //cout<<"**************Root : "<<root<<"***********";
        past_path.push(curr_dir);
        clearStack(forw_path);
        string parent_path = curr_dir;
        int pos = parent_path.find_last_of("/\\");
        parent_path = parent_path.substr(0, pos);
        strcpy(curr_dir, parent_path.c_str());
        listDir(curr_dir);
        cursor_x = 1, cursor_y = 1;
        printCursor();
    }
    // cout << "backspace \n";
}

void enter()
{
    string fileName = d_nameList[cur_window + cursor_x - 1];
    string fullPath;
    if (sflag == 1)
    {
        fullPath = fileName;
    }
    else
    {
        fullPath = string(curr_dir) + "/" + fileName;
    }

    char *path = new char[fullPath.length() + 1];
    strcpy(path, fullPath.c_str());
    //cout<<"**************"<<path<<"************";

    struct stat sb;
    stat(path, &sb);

    //If file type is Directory
    if ((sb.st_mode & S_IFMT) == S_IFDIR)
    {
        //cout<<"DIR"<<endl;
        cursor_x = 1;
        sflag = 0;
        if (fileName == string("."))
        {
        }
        else if (fileName == string(".."))
        {
            past_path.push(string(curr_dir));
            clearStack(forw_path);
            string parent_path = curr_dir;
            int pos = parent_path.find_last_of("/\\");
            parent_path = parent_path.substr(0, pos);
            strcpy(curr_dir, parent_path.c_str());
        }
        else
        {
            if (curr_dir != NULL)
            {
                past_path.push(string(curr_dir));
                clearStack(forw_path);
            }
            strcpy(curr_dir, path);
        }

        listDir(curr_dir);
    }
    //If file type is Regular File
    else if ((sb.st_mode & S_IFMT) == S_IFREG)
    {
        //cout<<"**************File Path : "<<filepath<<"***************"<<endl;
        int fileOpen = open("/dev/null", O_WRONLY);
        dup2(fileOpen, 2);
        close(fileOpen);
        pid_t processID = fork();
        if (processID == 0)
        {
            execlp("xdg-open", "xdg-open", path, NULL);
            exit(0);
        }
    }
    // else
    // {
    //     showError("Unknown File !!! :::::" + string(curDir));
    // }
}

void home()
{
    string cpath = string(curr_dir);
    if (cpath != string(root))
    {
        if (sflag != 1)
            past_path.push(string(curr_dir));
        clearStack(forw_path);
        strcpy(curr_dir, root);
        sflag = 0;
        listDir(curr_dir);
        cursor_x = 1, cursor_y = 1;
        printCursor();
        // printf("%c[%d;%dH",27,5,1);
        // printf("%c[2K", 27);
    }
    // cout << "home\n";
}

void deleteLastChar()
{
    int last_row = terminal.ws_row;
    printf("%c[%d;%dH", 27, last_row, cursor_y);
    printf("%c[0K", 27);
    // cout << ": ";
}

void clearLastLine()
{
    int last_row = terminal.ws_row;
    printf("%c[%d;%dH", 27, last_row, 1);
    printf("%c[0K", 27);
    cout << ": ";
}

void splitCommand(string fullCommand)
{
    if (fullCommand.length() == 0)
    {
        cout << "commadn too shorrt\n";
        return;
    }
    string temp = "";
    int len = fullCommand.size();
    // cout << "full command : " << fullCommand<<endl;
    command_tokens.clear();
    for (int i = 0; i < len; i++)
    {
        if (fullCommand[i] == ' ' || fullCommand[i] == '\n')
        {
            if (temp.size() > 0)
            {
                command_tokens.push_back(temp);
            }
            temp = "";
        }
        else if (fullCommand[i] == '\\')
        {
            i++;
            temp + fullCommand[i];
        }
        else
        {
            temp += fullCommand[i];
        }
    }
    return;
}

int enterCommandMode()
{
    int last_row = terminal.ws_row;
    string fullCommand;
    string firstWord;
    char ch;

    do
    {
        cursor_y = 2;
        printf("%c[%d;%dH", 27, last_row, 1);
        printf("%c[2K", 27);
        cout << ": ";
        command_tokens.clear();
        fullCommand = "";
        while ((ch = getchar()) != 27 && ch != 10)
        {
            cout << ch;
            cursor_y++;
            if (ch == 127)
            {
                if (cursor_y > 3)
                {
                    cursor_y--;
                    deleteLastChar();
                    if (fullCommand.size() <= 1)
                    {
                        fullCommand = "";
                    }
                    else
                    {
                        fullCommand = fullCommand.substr(0, fullCommand.length() - 1);
                    }

                    cursor_y--;
                }
                // fullComma
            }
            else
            {
                // cout << ch;
                fullCommand += ch;
            }
            // ch = getchar();
        }
        fullCommand += "\n";
        cout << "\n";
        splitCommand(fullCommand);
        // cout << "checking commands vecro\n";
        // for (int i = 0; i < command_tokens.size(); i++)
        // {
        //     cout << command_tokens[i] << endl;
        // }

        // cout << "\nfirst wp  rd : " << firstWord << endl;
        if (ch == 10)
        {
            firstWord = command_tokens[0];
            if (firstWord == "copy")
                copy();
            else if (firstWord == "move")
                move();
            else if (firstWord == "rename")
                renameFunc();
            else if (firstWord == "create_file")
                create_file();
            else if (firstWord == "create_dir")
                create_dir();
            else if (firstWord == "delete_file")
                delete_file();
            else if (firstWord == "delete_dir")
                delete_dir();
            else if (firstWord == "goto")
                goto_path();
            else if (firstWord == "search")
                search();
            else
            {
                cursor_x = terminal.ws_row;
                cursor_y = 1;
                printCursor();
                printf("\x1b[0K");
                printf(":");
                cout << "command not found." << endl;
            }
        }
    } while (ch != 27);
    cout << "command mode\n";
    return 0;
}

int copy()
{
    // command_tokens.clear();
    int len;
    string source_filename, dest_path2;
    if (command_tokens.size() < 3)
    {
        cout << "invalid command\n";
    }
    else
    {
        dest_path = command_tokens[command_tokens.size() - 1];
        dest_path = get_absolute_path(dest_path);
        // cout << "dest path : " << dest_path<<endl;
        if (isDirectory(dest_path))
        {
            for (int i = 1; i < command_tokens.size() - 1; i++)
            {

                source_path = get_absolute_path(command_tokens[i]);
                len = source_path.length();
                int pos = source_path.find_last_of("/\\");
                source_filename = source_path.substr(pos + 1, len - pos);
                // cout << "source filename : " << source_filename<<endl;
                dest_path2 = dest_path + "/" + source_filename;
                if (isDirectory(source_path))
                {
                    copyDirectory(source_path, dest_path2);
                }
                else
                {
                    // cout << "copying : " << command_tokens[i] << endl;
                    copyFile(source_path, dest_path2);
                }
            }
        }
        else
        {
            cout << "destination must be a folder";
            return -1;
        }
    }
    // cout << "in copy\n";
    return 0;
}

void copyFile(string source, string dest)
{
    char buf[BUF_SIZE];
    int bufsize;
    FILE *src, *dst;
    // cout << "source : " << source<< "  dest : " << dest << endl;
    src = fopen(source.c_str(), "r");
    if (src == NULL)
    {
        cout << "error in copying\n";
        return;
    }
    dst = fopen(dest.c_str(), "w");
    if (dst == NULL)
    {
        cout << "error in copying\n";
        return;
    }

    size_t in, out;
    while (1)
    {
        in = fread(buf, 1, bufsize, src);
        if (0 == in)
            break;
        out = fwrite(buf, 1, in, dst);
        if (0 == out)
            break;
    }

    struct stat source_stat;
    stat(source.c_str(), &source_stat);
    chown(dest.c_str(), source_stat.st_uid, source_stat.st_gid);
    chmod(dest.c_str(), source_stat.st_mode);
    fclose(src);
    fclose(dst);
    return;
}

void copyDirectory(string source, string dest)
{
    if (mkdir(dest.c_str(), 0755) != 0)
    {
        cout << "error in copy(creating) direct\n";
        // perror("");
        return;
    }
    DIR *d;
    d = opendir(source.c_str());
    if (d == NULL)
    {
        cout << "error in opendir\n";
        return;
    }
    struct dirent *dir;
    while ((dir = readdir(d)))
    {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            continue;
        string src_path = source + "/" + string(dir->d_name);
        string d_path = dest + "/" + string(dir->d_name);

        if (isDirectory(src_path))
        {
            copyDirectory(src_path, d_path);
        }
        else
        {
            copyFile(src_path, d_path);
        }
    }
}

void renameFunc()
{
    if (command_tokens.size() != 3)
    {
        cout << "invalid command \n";
        return;
    }
    string old_path = get_absolute_path(command_tokens[1]);
    string new_path = get_absolute_path(command_tokens[2]);
    if (rename(old_path.c_str(), new_path.c_str()) == -1)
        cout << "error in renaming";
    return;
}

void goto_path()
{
    // is_goto_called = true;
    if (command_tokens.size() != 2)
    {
        cout << "invalid command \n";
        return;
    }
    string dest = command_tokens[1];
    dest_path = get_absolute_path(dest);
    if (!isDirectory(dest_path))
    {
        cout << "not a directory\n";
        return;
    }
    past_path.push(curr_dir);
    strcpy(curr_dir, dest_path.c_str());
    listDir(curr_dir);
}

void create_dir()
{
    if (command_tokens.size() < 3)
    {
        cout << "invalid command \n";
        return;
    }
    string dest_folder_path = get_absolute_path(command_tokens[command_tokens.size() - 1]);
    if (!isDirectory(dest_folder_path))
    {
        cout << "not a directory \n";
        return;
    }
    for (int i = 1; i < command_tokens.size() - 1; i++)
    {
        dest_path = dest_folder_path + "/" + command_tokens[i];
        // cout << "dest path :" << dest_path<<endl;
        mkdir(dest_path.c_str(), 0755);
    }
    return;
    // cout << "create dir \n";
}

void create_file()
{
    if (command_tokens.size() < 3)
    {
        cout << "invalid command \n";
        return;
    }
    string dest_folder_path = get_absolute_path(command_tokens[command_tokens.size() - 1]);
    if (!isDirectory(dest_folder_path))
    {
        cout << "not a directory \n";
        return;
    }
    for (int i = 1; i < command_tokens.size() - 1; i++)
    {
        dest_path = dest_folder_path + "/" + command_tokens[i];
        if (creat(dest_path.c_str(), S_IRGRP | S_IROTH | S_IRWXU) < 0)
        {
            cout << "error creating file";
        }
    }
    return;
    // cout << "create file \n ";
}

void delete_file()
{
    // cout <<"inn here"<<endl;
    if (command_tokens.size() < 2)
    {
        cout << "invalid command \n";
        return;
    }
    for (int i = 1; i < command_tokens.size(); i++)
    {
        dest_path = get_absolute_path(command_tokens[i]);
        if (remove(dest_path.c_str()) != 0)
        {
            cout << "error deleting file";
        }
    }
    return;
    // cout << "delete file \n";
}

void delete_dir()
{
    // cout << "oin delete dir \n";
    if (command_tokens.size() < 2)
    {
        cout << "invalid command \n";
        return;
    }
    for (int i = 1; i < command_tokens.size(); i++)
    {
        dest_path = get_absolute_path(command_tokens[i]);
        if (!isDirectory(dest_path))
        {
            cout << "not a directory \n";
            return;
        }
        recursiveDelete(dest_path);
    }
    // cout << "delete dir \n ";
}

void recursiveDelete(string path)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(path.c_str());
    if (d == NULL)
    {
        perror("opendir");
        return;
    }

    while (dir = readdir(d))
    {
        if ((string(dir->d_name) == "..") || (string(dir->d_name) == "."))
            continue;
        else
        {
            string path_to_delete = path + "/" + string(dir->d_name);
            if (isDirectory(path_to_delete))
                recursiveDelete(path_to_delete);
            else
                // cout << "path to delete : " << path_to_delete << endl;
                remove(path_to_delete.c_str());
        }
    }
    closedir(d);
    rmdir(path.c_str());
    return;
}

void move()
{
    string file_to_move, source_filename,dest_path2;
    if (command_tokens.size() < 2)
    {
        cout << "invalid command \n";
        return;
    }
    dest_path = get_absolute_path(command_tokens[command_tokens.size() - 1]);

    for (int i = 1; i < command_tokens.size() - 1; i++)
    {
        source_path = get_absolute_path(command_tokens[i]);
        int len = source_path.length();
        int pos = source_path.find_last_of("/\\");
        source_filename = source_path.substr(pos + 1, len - pos);
        dest_path2 = dest_path + "/" + source_filename;
        file_to_move = get_absolute_path(command_tokens[i]);
        if (isDirectory(file_to_move))
        {
            copyDirectory(file_to_move, dest_path2);
            recursiveDelete(file_to_move);
        }
        else
        {
            copyFile(file_to_move, dest_path2);
            if (remove(file_to_move.c_str()) != 0){
                cout << "error in moving file \n";
                return;
            }
        }
    }
}

void search()
{
    cout << "searcgh\n";
}