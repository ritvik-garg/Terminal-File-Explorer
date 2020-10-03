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
using namespace std;
#define MAX_CHAR 4096

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
std::stack<string> past_path;
std::stack<string> forw_path;

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
    // printf("%c[?1049h",27); //Alternate buffer on
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
    printf("%c[%d;%dH", 27, 1, 1);
    // printf("\x1b[31m\x1b[44m");
    closedir(dr);
}

void modify_dlist()
{
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
            enterCommandMode();
        else if (ch == 'q')
        {
            cout << "exit()";
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

void deleteLastChar(){
    int last_row = terminal.ws_row;
    printf("%c[%d;%dH", 27, last_row, cursor_y);
    printf("%c[0K",27);
    cout << ": ";    
}

int enterCommandMode()
{
    string fullCommand;
    char ch = getchar();
    while(ch!=27){
        if(ch==127){
            deleteLastChar();
            // fullComma
        }
        else cout<<ch;
    }
    cout << "command mode\n";
    return 0;
}