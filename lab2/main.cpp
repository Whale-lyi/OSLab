#include "iostream"
#include "string"
#include "vector"
#include "cstring"

using namespace std;

typedef unsigned char u8;   // 1 byte
typedef unsigned short u16; // 2 byte
typedef unsigned int u32;   // 4 byte

// nasm function
extern "C"
{
    void my_printWhite(const char * s, int len);
    void my_printRed(const char * s, int len);
}

class FileOrDirNode {
    string name;
    string path;
    bool isFile;       // 文件属性
    u32 fileSize;      // 文件的大小，如果是目录的话，值为0
    u16 firstClus;     // 文件或目录的起始簇
    u32 directFileNum; // 文件直接子文件数量
    u32 directDirNum;   // 文件直接子目录数量
    FileOrDirNode * parent;
    vector<FileOrDirNode *> childNodes;
public:
    virtual ~FileOrDirNode() {
        for(auto node : childNodes) {
            if (node != nullptr) {
                delete node;
                node = nullptr;
            }
        }
        childNodes.clear();
    }

    FileOrDirNode *getParent() const {
        return parent;
    }

    void setParent(FileOrDirNode *parent) {
        FileOrDirNode::parent = parent;
    }

    const vector<FileOrDirNode *> &getChildNodes() const {
        return childNodes;
    }

    void addChildNode(FileOrDirNode * child) {
        childNodes.push_back(child);
    }

    const string &getName() const {
        return name;
    }

    void setName(const string &name) {
        FileOrDirNode::name = name;
    }

    const string &getPath() const {
        return path;
    }

    void setPath(const string &path) {
        FileOrDirNode::path = path;
    }

    bool getIsFile() const {
        return isFile;
    }

    void setIsFile(bool isFile) {
        FileOrDirNode::isFile = isFile;
    }

    u32 getFileSize() const {
        return fileSize;
    }

    void setFileSize(u32 fileSize) {
        FileOrDirNode::fileSize = fileSize;
    }

    u16 getFirstClus() const {
        return firstClus;
    }

    void setFirstClus(u16 firstClus) {
        FileOrDirNode::firstClus = firstClus;
    }

    u32 getDirectFileNum() const {
        return directFileNum;
    }

    void setDirectFileNum(u32 directFileNum) {
        FileOrDirNode::directFileNum = directFileNum;
    }

    u32 getDirectDirNum() const {
        return directDirNum;
    }

    void setDirectDirNum(u32 directDirNum) {
        FileOrDirNode::directDirNum = directDirNum;
    }
};

#pragma pack(1) /*指定按1字节对齐*/
// 引导扇区, 偏移量11字节, 长度25字节
struct BPB {
    u16 BPB_BytsPerSec; //每扇区字节数
    u8 BPB_SecPerClus;  //每簇扇区数
    u16 BPB_RsvdSecCnt; //Boot Record占用的扇区数
    u8 BPB_NumFATs;     //FAT表个数
    u16 BPB_RootEntCnt; //根目录区最大文件数
    u16 BPB_TotSec16;   //总扇区数
    u8 BPB_Media;       //介质描述符
    u16 BPB_FATSz16;    //一个FAT表占用的扇区数
    u16 BPB_SecPerTrk;  //每磁道扇区数
    u16 BPB_NumHeads;   //磁头数/面数
    u32 BPB_HiddSec;    //隐藏扇区数
    u32 BPB_TotSec32;   //如果BPB_TotSec16为0，该值为总扇区数
};

// 根目录条目, 32字节
struct RootEntry {
    char DIR_Name[11]; // 文件名8字节, 扩展名3字节
    u8 DIR_Attr;       // 文件属性——目录:0x10 文件:0x20
    u8 reserve[10];    // 保留位
    u16 DIR_WrtTime;   // 最后一次写入时间
    u16 DIR_WrtDate;   // 最后一次写入日期
    u16 DIR_FstClus;   // 此条目开始簇号
    u32 DIR_FileSize;  // 文件大小
};

#pragma pack() /*取消指定对齐，恢复缺省对齐*/

// 工具函数
void printRed(const string& s);
void printStr(const string& s);
void readBPB(FILE * fat12);
void readRootEntry(FILE * fat12);
void readNode(FILE * fat12, FileOrDirNode * root, int offset);
string getFileName(RootEntry * rootEntry);
void readChildEntries(FILE * fat12, FileOrDirNode * parent, int startClus);
int getNextCluster(FILE * fat12, int curClus);
vector<string> split(const string& str, const string& delim);
bool checkCommand(const vector<string>& command);
void doLS(const vector<string>& command);
void doCAT(const vector<string>& command, FILE * fat12);
FileOrDirNode * parsePath(string path);
string &trim(string& s);
void outputLSL(FileOrDirNode * node);
void outputLS(FileOrDirNode * node);
// 全局变量
BPB * bpb_ptr;
FileOrDirNode * root_ptr;
int rootStartSectors = 0; // 根目录区起始扇区
int dataStartSectors = 0; // 数据区起始扇区
bool isLSL = false;

int main() {
    FILE * fat12 = fopen("./a2.img", "rb");
    // 读取引导扇区和根目录区, 生成目录树
    readBPB(fat12);
    readRootEntry(fat12);
    // 读取用户输入
    string input;
    while (true) {
        isLSL = false;
        printStr("> ");
        getline(cin, input);
        if (input.length() == 0) {
            printStr("Please input command!\n");
            continue;
        }
        if (input == "exit") {
            break;
        }
        vector<string> command = split(input, " ");
        // 检查命令是否符合格式, 此处不处理路径解析
        if (!checkCommand(command)) {
            continue;
        }
        if (command[0] == "cat") {
            doCAT(command, fat12);
        } else if (command[0] == "ls") {
            doLS(command);
        }
    }

    delete root_ptr;
    root_ptr = nullptr;
    free(bpb_ptr);
    fclose(fat12);
}

void doCAT(const vector<string>& command, FILE * fat12) {
    FileOrDirNode * node = parsePath(command[1]);
    if (node == nullptr) {
        return;
    } else if (!node->getIsFile()) {
        printStr("Parameter should be a file!\n");
        return;
    }
    int curClus = node->getFirstClus();
    int bytsPerClus = bpb_ptr->BPB_SecPerClus * bpb_ptr->BPB_BytsPerSec;
    do {
        if (curClus == 0 || curClus == 1 || curClus >= 0xFF7) {
            break;
        }
        int nextClus = getNextCluster(fat12, curClus);
        int offset = dataStartSectors * bpb_ptr->BPB_BytsPerSec + (curClus - 2) * bytsPerClus;
        char * content = new char[bytsPerClus + 1];
        fseek(fat12, offset, SEEK_SET);
        fread(content, 1, bytsPerClus, fat12);
        content[bytsPerClus] = '\0';
        printStr(content);
        curClus = nextClus;
    } while (curClus < 0xFF7);
    printStr("\n");
}

/**
 * 处理ls指令
 * @param command
 */
void doLS(const vector<string>& command) {
    FileOrDirNode * node = root_ptr;
    for (int i = 1; i < command.size(); ++i) {
        if (command[i][0] != '-') {
            node = parsePath(command[i]);
            if (node == nullptr) {
                return;
            } else if (node->getIsFile()) {
                printStr("Parameter should be a directory!\n");
                return;
            }
            break;
        }
    }

    if (isLSL) {
        // -l
        outputLSL(node);
    } else {
        outputLS(node);
    }
}

/**
 * 递归打印LSL
 * @param node
 */
void outputLSL(FileOrDirNode * node) {
    printStr(node->getPath() + " " + to_string(node->getDirectDirNum()) + " " + to_string(node->getDirectFileNum()) + ":\n");
    printRed(".\n");
    printRed("..\n");
    for (auto i : node->getChildNodes()) {
        if (i->getIsFile()) {
            // 文件
            printStr(i->getName() + " ");
            printStr(to_string(i->getFileSize()) + "\n");
        } else {
            printRed(i->getName() + " ");
            printStr(to_string(i->getDirectDirNum()) + " " + to_string(i->getDirectFileNum()) + "\n");
        }
    }
    printStr("\n");
    // 对子结点是目录的情况递归调用
    for (auto i : node->getChildNodes()) {
        if (!i->getIsFile()) {
            outputLSL(i);
        }
    }
}

/**
 * 递归打印LS
 * @param node
 */
void outputLS(FileOrDirNode * node) {
    printStr(node->getPath() + ":\n");
    printRed(".  ..  ");
    for (auto i : node->getChildNodes()) {
        if (i->getIsFile()) {
            // 文件
            printStr(i->getName() + "  ");
        } else {
            printRed(i->getName() + "  ");
        }
    }
    printStr("\n");
    // 对子结点是目录的情况递归调用
    for (auto i : node->getChildNodes()) {
        if (!i->getIsFile()) {
            outputLS(i);
        }
    }
}

/**
 * 解析路径返回节点
 * @param path
 * @return 路径有错返回nullptr
 */
FileOrDirNode * parsePath(string path) {
    FileOrDirNode * cur_node = root_ptr;
    if (path == "/") {
        return root_ptr;
    } else {
        path = trim(path);
        if (path[0] != '/') {
            path.insert(0, "/");
        }
        vector<string> path_param = split(path, "/");
        for (auto & i : path_param) {
            if (i == ".") {
                continue;
            } else if (i == "..") {
                cur_node = cur_node->getParent();
            } else {
                bool find_flg = false;
                // 对当前节点子结点遍历, 查找名称相同项
                for (int j = 0; j < cur_node->getChildNodes().size(); ++j) {
                    if (cur_node->getChildNodes()[j]->getName() == i) {
                        find_flg = true;
                        cur_node = cur_node->getChildNodes()[j];
                    }
                }
                if (!find_flg) {
                    printStr("Wrong path!\n");
                    return nullptr;
                }
            }
        }
        return cur_node;
    }
}

/**
 * 去除首尾空格
 * @param s
 * @return
 */
string &trim(string& s) {
    if (s.empty()){
        return s;
    }
    s.erase(0,s.find_first_not_of(' '));
    s.erase(s.find_last_not_of(' ') + 1);
    return s;
}

/**
 * 检查命令是否符合要求
 * @param command
 * @return 不符合要求返回false
 */
bool checkCommand(const vector<string>& command) {
    bool result = true;
    if (command[0] == "ls") {
        int cnt = 0;
        for (int i = 1; i < command.size(); ++i) {
            // -l
            if (command[i][0] == '-') {
                for (int j = 1; j < command[i].length(); ++j) {
                    if (command[i][j] != 'l') {
                        printStr("Unknown parameter!\n");
                        result = false;
                        break;
                    }
                }
                if (!result) {
                    isLSL = false;
                    break;
                }
                isLSL = true;
            } else {
                // path
                cnt++;
            }
        }
        if (cnt > 1) {
            printStr("Too many paths!\n");
            result = false;
        }

    } else if (command[0] == "cat"){
        if (command.size() != 2) {
            printStr("There should be two parameters!\n");
            result = false;
        }
    } else {
        printStr("Unknown command!\n");
        result = false;
    }

    return result;
}

/**
 * 分割字符串
 * @param str
 * @param delim
 * @return
 */
vector<string> split(const string& str, const string& delim) {
    vector <string> res;
    if (str.empty()) return res;
    //先将要切割的字符串从string类型转换为char*类型
    char *strs = new char[str.length() + 1]; //不要忘了\0
    strcpy(strs, str.c_str());

    char *d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());

    char *p = strtok(strs, d);
    while (p) {
        string s = p; //分割得到的字符串转换为string类型
        res.push_back(s); //存入结果数组
        p = strtok(nullptr, d);
    }

    return res;
}

/**
 * 读取引导扇区
 * @param fat12
 */
void readBPB(FILE * fat12) {
    bpb_ptr = (struct BPB *) malloc(sizeof(struct BPB));
    fseek(fat12, 11, SEEK_SET);
    fread(bpb_ptr, 1, 25, fat12);
    // 根目录起始扇区 = 引导占用扇区数 + FAT个数*单个占用扇区
    rootStartSectors = bpb_ptr->BPB_RsvdSecCnt + bpb_ptr->BPB_NumFATs * bpb_ptr->BPB_FATSz16;
    // 数据起始扇区 = 根目录起始 + 根目录所占扇区数
    dataStartSectors = rootStartSectors + (bpb_ptr->BPB_BytsPerSec - 1 + bpb_ptr->BPB_RootEntCnt * 32) / bpb_ptr->BPB_BytsPerSec;
}

/**
 * 读取根目录下的文件
 */
void readRootEntry(FILE * fat12) {
    if (root_ptr != nullptr) {
        delete root_ptr;
        root_ptr = nullptr;
    }
    root_ptr = new FileOrDirNode;
    root_ptr->setName("/");
    root_ptr->setPath("/");
    root_ptr->setDirectDirNum(0);
    root_ptr->setDirectFileNum(0);
    root_ptr->setIsFile(false);
    root_ptr->setFileSize(0);
    root_ptr->setFirstClus(0);
    root_ptr->setParent(root_ptr);
    int offset = rootStartSectors * bpb_ptr->BPB_BytsPerSec;
    for (int i = 0; i < bpb_ptr->BPB_RootEntCnt; ++i) {
        readNode(fat12, root_ptr, offset);
        offset += 32;
    }
}

/**
 * 读取根文件/目录节点
 */
void readNode(FILE * fat12, FileOrDirNode * root, int offset) {
    RootEntry * rootEntryPtr = (struct RootEntry *) malloc(sizeof(struct RootEntry));
    fseek(fat12, offset, SEEK_SET);
    fread(rootEntryPtr, 1, 32, fat12);
    // 读到空文件
    if (rootEntryPtr->DIR_Name[0] == '\0') {
        free(rootEntryPtr);
        return;
    }
    // 判断文件名是否符合要求
    for (char tmp : rootEntryPtr->DIR_Name) {
        if (!((tmp >= 'A' && tmp <= 'Z') || (tmp >= '0' && tmp <= '9') || tmp == ' ')) {
            free(rootEntryPtr);
            return;
        }
    }

    FileOrDirNode * node = new FileOrDirNode;
    node->setParent(root);
    node->setFileSize(rootEntryPtr->DIR_FileSize);
    node->setFirstClus(rootEntryPtr->DIR_FstClus);
    node->setDirectFileNum(0);
    node->setDirectDirNum(0);
    node->setName(getFileName(rootEntryPtr));
    if (rootEntryPtr->DIR_Attr == 0x10) {
        // 目录
        node->setIsFile(false);
        root->addChildNode(node);
        root->setDirectDirNum(root->getDirectDirNum() + 1);
        string path = root->getPath();
        path += node->getName();
        path += '/';
        node->setPath(path);
        // 递归读取子目录下的文件
        readChildEntries(fat12, node, node->getFirstClus());
    } else if (rootEntryPtr->DIR_Attr == 0x20){
        // 文件
        node->setIsFile(true);
        root->addChildNode(node);
        root->setDirectFileNum(root->getDirectFileNum() + 1);
        string path = root->getPath();
        path += node->getName();
        node->setPath(path);
    }
    free(rootEntryPtr);
}

/**
 * 读取子目录文件
 */
void readChildEntries(FILE * fat12, FileOrDirNode * parent, int startClus) {
    int curClus = startClus;
    do {
        if (curClus == 0 || curClus == 1 || curClus >= 0xFF7) {
            break;
        }
        int nextClus = getNextCluster(fat12, curClus);
        int offset = dataStartSectors * bpb_ptr->BPB_BytsPerSec + (curClus - 2) * bpb_ptr->BPB_SecPerClus * bpb_ptr->BPB_BytsPerSec;
        for (int i = 0; i < bpb_ptr->BPB_SecPerClus * bpb_ptr->BPB_BytsPerSec; i += 32) {
            readNode(fat12, parent, offset + i);
        }
        curClus = nextClus;
    } while (curClus < 0xFF7);
}

/**
 * 获取当前簇的下一个簇号
 * @param fat12
 * @param curClus
 * @return
 */
int getNextCluster(FILE * fat12, int curClus) {
    int FATBase = bpb_ptr->BPB_RsvdSecCnt * bpb_ptr->BPB_BytsPerSec;
    // 使用2字节来存储FAT中的值
    int num1 = 0, num2 = 0; // 初始化
    int *p1 = &num1;
    int *p2 = &num2;
    int start = curClus * 3 / 2; // 当前偶数簇对应到FAT中是从第几个字节开始, 奇数簇+1

    fseek(fat12, FATBase + start, SEEK_SET);
    fread(p1, 1, 1, fat12);
    fread(p2, 1, 1, fat12);
    if (curClus % 2 == 0) {
        // 偶数簇前有start个字节
        return num1 + ((num2 & 0x0f) << 8);
    } else {
        // 奇数簇前有start + 0.5个字节
        return ((num1 & 0xf0) >> 4) + (num2 << 4);
    }
}

/**
 * 获取文件名
 */
string getFileName(RootEntry * rootEntry) {
    string name;
    for (int i = 0; i < 8; ++i) {
        if (rootEntry->DIR_Name[i] != ' ') {
            name += rootEntry->DIR_Name[i];
        }
    }
    if (rootEntry->DIR_Attr == 0x20) {
        name += '.';
        for (int i = 8; i < 11; ++i) {
            if (rootEntry->DIR_Name[i] != ' ') {
                name += rootEntry->DIR_Name[i];
            }
        }
    }
    return name;
}

void printRed(const string& s) {
    // cout << "\033[31m" << s << "\033[0m";
   my_printRed(s.data(), s.size());
}

void printStr(const string& s) {
    // cout << s;
   my_printWhite(s.data(), s.size());
}
