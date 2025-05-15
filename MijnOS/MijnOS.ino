/*
    Tobias de Bildt
    TINBES-03
    04-07-2023
    Wouter Bergmann-Tiest
*/

#include <EEPROM.h>
#include "instruction_set.h"

//
// Initializing all components
//

// Command Line Interface
const int BUFSIZE = 12;
static char CLIBuffer[4][BUFSIZE];
static int cliBufferCounterArguments = 0;
static int cliBufferCounter = 0;

// File Management
struct FATEntry {
    char name[12];
    int addr;
    int size;
};

const int FATSIZE = 10;
int noOfFiles;
FATEntry FAT[FATSIZE];

// Memory Management
struct memVar {
    byte name;
    int type;
    int length;
    int adress;
    int procID;
};
const int MEMORY_TABLE_SIZE = 10;
const int MEMORY_MAX_SIZE = sizeof(memVar) * MEMORY_TABLE_SIZE;
int noOfVars = 0;
memVar memoryTable[MEMORY_TABLE_SIZE];
byte RAM[MEMORY_MAX_SIZE];

// Process Management
struct process {
    char name[12];
    int procID;
    char state;
    int pc;
    int fp;
    int sp;
    int address;
};
const int PROCESS_TABLE_SIZE = 10;
int noOfProc;
int processCounter = 0;
process processTable[PROCESS_TABLE_SIZE];

// Stack Management
const int STACKSIZE = 16;
byte stack[PROCESS_TABLE_SIZE][STACKSIZE] = {0};

void store();
void retrieve();
void erase();
void files();
void freespace();
void run();
void list();
void suspend();
void resume();
void kill();
void clear();

typedef struct {
    char name[BUFSIZE];
    void (*func)();
    int noOfArgs;
} commandType;

static commandType commandList[] = {
    {"store", &store, 2},
    {"retrieve", &retrieve, 1},
    {"erase", &erase, 1},
    {"files", &files, 0},
    {"freespace", &freespace, 0},
    {"run", &run, 1},
    {"list", &list, 0},
    {"suspend", &suspend, 1},
    {"resume", &resume, 1},
    {"kill", &kill, 1},
    {"clear", &clear, 0},
};

static int commandListSize = sizeof(commandList) / sizeof(commandType);

// 
// COMMAND LINE STUFF
// 

// Check whether command is known
bool validateCliInput() {
    bool match = false;
    for (int i = 0; i < commandListSize; i++) {
        // Check if listArgument is the argument given
        if (strcmp(commandList[i].name, CLIBuffer[0]) == 0) {
            // Check if the right amount of arguments have been given
            if (cliBufferCounterArguments != commandList[i].noOfArgs) {
                Serial.print(F("This command requires: "));
                Serial.print(commandList[i].noOfArgs);
                Serial.println(F(" arguments."));
            } else {
                match = true;
                commandList[i].func();
            }
        }
    }
    // No match for command
    if (!match) {
          Serial.println("This command does not exist...");
          Serial.println("These commands exist:");
          for(auto command : commandList){
            Serial.println(command.name);
          }
    }
    return match;
}

// Function that will clear the CLI buffer
void clearCliBuffer(){
  for (int i = 0; i < 4; i++) {
    // Set buffer to null
    memset(CLIBuffer, 0, BUFSIZE);
  }

  cliBufferCounter = 0;
  cliBufferCounterArguments = 0;
}

// Function that updates input buffer
void commandLine() {
    // Wait for available data
    if (Serial.available() > 0) {
        int character = Serial.read();
        // Separate arguments from command when space is pressed
        if (character == ' ') {
            cliBufferCounterArguments++;
            cliBufferCounter = 0;
        } else if (character == 13 || character == 10) {
            // Enter pressed, check command and clear buffer
            delayMicroseconds(1042);
            CLIBuffer[cliBufferCounterArguments][cliBufferCounter] = '\0';
            Serial.read();
            validateCliInput();
            clearCliBuffer();
        } else {
            // Add char to buffer
            CLIBuffer[cliBufferCounterArguments][cliBufferCounter] = character;
            cliBufferCounter++;
        }
    }
}
// Function validates input on numbers
bool isNumeric() {
    for (int i = 0; CLIBuffer[1][i] != '\0'; i++) {
        if (int(CLIBuffer[1][i]) < 48 || int(CLIBuffer[1][i]) > 57) {
            return false;
        }
    }
    return true;
}

// 
// FILE SYSTEM STUFF
// 

// Write FAT entry on index
void writeFATEntry(int index, const FATEntry& entry) {
    int address = sizeof(noOfFiles) + (index * sizeof(FATEntry));
    EEPROM.put(address, entry);
}

// Read FAT entry on index
FATEntry readFATEntry(int index) {
    FATEntry entry;
    int address = sizeof(noOfFiles) + (index * sizeof(FATEntry));
    EEPROM.get(address, entry);
    return entry;
}


// Write entire FAT to EEPROM
void writeFAT() {
    int address = 0;
    EEPROM.put(address, noOfFiles);
    address += sizeof(noOfFiles);
    for (int i = 0; i < noOfFiles; i++) {
        EEPROM.put(address, FAT[i]);
        address += sizeof(FATEntry);
    }
}
// Read FAT from EEPROM
void readFAT() {
    int address = 0;
    EEPROM.get(address, noOfFiles);
    address += sizeof(noOfFiles);
    for (int i = 0; i < FATSIZE; i++) {
        FAT[i] = readFATEntry(i);
    }
}

int findFATEntry(const char* fileName) {
    readFAT();
    for (int i = 0; i < noOfFiles; i++) {
        if (strcmp(FAT[i].name, fileName) == 0) {
            return i;
        }
    }
    return -1;
}

void storeFile(const char* fileName, int fileSize) {
    Serial.println(F("Storing: "));
    Serial.println(fileName);

    readFAT();

    // 1: Check for if there is an available FAT entry
    if (noOfFiles >= FATSIZE) {
        Serial.println(F("FAT is full"));
        return;
    }

    // 2: Check if the filename is available
    if (findFATEntry(fileName) != -1) {
    Serial.println(F("File exists"));
    return;
    }

    // 3: Sort the FAT table in preparation if there is enough space
    sort();

    // 4: Check if there is enough space
    int position = searchSpace(fileSize);
    if (position == -1) {
        Serial.println(F("Error: No space left for file."));
        return;
    }

    Serial.println(F("Give input for file:"));
    char fileData[fileSize];
    // Wait for data
    while (Serial.available() == 0) {
        
    }

    for (int i = 0; i < fileSize; i++) {
        if (Serial.available() != 0) {
            // Add data to filedata
            fileData[i] = Serial.read();
        } else {
            fileData[i] = 32;
        }
        delayMicroseconds(1042);
    }

    // Clear Serial buffer
    while (Serial.available()) {
        Serial.read();
        delayMicroseconds(1042);
    }

    FATEntry file = {};
    strcpy(file.name, fileName);
    file.addr = position;
    file.size = fileSize;

    FAT[noOfFiles] = file;
    noOfFiles++;
    sort();
    writeFAT();

    fileSize++;
    for (int i = 0; i < fileSize; i++) {
        EEPROM.write(position, fileData[i]);
        position++;
    }

    Serial.println(F("File has been stored."));
}

// Sort File table
void sort() {
    bool sorted = false;
    while (!sorted) {
        sorted = true;
        for (int i = 0; i < noOfFiles - 1; i++) {
            if (FAT[i].addr > FAT[i + 1].addr) {
                FATEntry temp = FAT[i];
                FAT[i] = FAT[i + 1];
                FAT[i + 1] = temp;
                sorted = false;
            }
        }
    }
}

// Function finds available position to store file
int searchSpace(int fileSize) {
    sort();
    // First block
    int systemMemory = (sizeof(FATEntry) * FATSIZE) + 1;
    if (FAT[0].addr - systemMemory >= fileSize) {
        return systemMemory;
    }

    // In between blocks
    for (int i = 0; i < noOfFiles - 1; i++) {
        int availableSpace = FAT[i + 1].addr - (FAT[i].addr + FAT[i].size);
        if (availableSpace >= fileSize) {
            return FAT[i].addr + FAT[i].size;
        }
    }

    // Behind last block
    int lastFileEnd = FAT[noOfFiles - 1].addr + FAT[noOfFiles - 1].size;
    if (EEPROM.length() - lastFileEnd >= fileSize) {
        if (noOfFiles == 0) {
            return lastFileEnd + systemMemory;
        }
        return lastFileEnd;
    }

    return -1;
}

void retrieveFile(const char* filename) {
    readFAT();

    // Check if file exists
    int fatIndex = findFATEntry(filename);
    if (fatIndex == -1) {
        Serial.println(F("File not found."));
        return;
    }

    int fileIndex = FAT[fatIndex].addr;

    Serial.print(F("\nFile contains: "));
    for (int i = 0; i < FAT[fatIndex].size; i++) {
        Serial.print((char)EEPROM.read(fileIndex));
        fileIndex++;
    }
    Serial.print(F("\n"));
    Serial.println(F("========END OF FILE========"));
}

void eraseFile(const char* fileName) {
    readFAT();

    int fatIndex = findFATEntry(fileName);
    if (fatIndex == -1) {
        Serial.println(F("File not found."));
        return;
    }
    // Move the other files up
    for (int i = fatIndex; i < noOfFiles; i++) {
        FAT[i] = FAT[i + 1];
    }
    // Empty out the last entry
    FATEntry emptyEntry;
    FAT[noOfFiles - 1] = emptyEntry;
    noOfFiles--;

    writeFAT();
    Serial.print(F("Erased: "));
    Serial.println(fileName);
}

void ls() {
    readFAT(); 

    Serial.print(noOfFiles);
    Serial.println(F(" file(s) in FAT:"));
    Serial.println(F("\nFilename\tSize\tAddress\n==============================================="));
    for(int i = 0; i < noOfFiles; i++){
        Serial.print(FAT[i].name);
        Serial.print("\t\t");
        Serial.print(FAT[i].size+1);
        Serial.print("\t");
        Serial.print(FAT[i].addr);
        Serial.println();
    }
    Serial.println();
}

void df() {
    int size = 1024 - ((sizeof(FATEntry) * FATSIZE) + 1);
  
    for(int i = 0; i<noOfFiles; i++){
        size -= FAT[i].size + 1;
    }
    Serial.print(F("Available bytes: "));Serial.println(size);
}

void clearEeprom() {
    Serial.println(F("Clearing EEPROM, this may take a while..."));
    for (int i = 0; i < EEPROM.length(); i++) {
        EEPROM.write(i, 0);
    }
    Serial.println(F("Completed EEPROM wipe :)"));
}


// 
// Stack Management
// 

// Functions for singular byte
void pushByte(int procID, int& sp, byte b) {
    stack[procID][sp++] = b; 
}
byte popByte(int procID, int& sp) {
    return stack[procID][--sp];
}

// Functions for Char
void pushChar(int procID, int& sp, char c) {
    pushByte(procID, sp, c);    // Push char
    pushByte(procID, sp, CHAR); // Push Type = char
}
char popChar(int procID, int& sp) {
    return popByte(procID, sp);
}

// Functions for int
void pushInt(int procID, int& sp, int i) {
    // Push integer itself
    pushByte(procID, sp, highByte(i));
    pushByte(procID, sp, lowByte(i));
    // Push var type
    pushByte(procID, sp, INT);
}
int popInt(int procID, int& sp) {
    byte lb = popByte(procID, sp);
    byte hb = popByte(procID, sp);
    int i = word(hb, lb);
    return i;
}

// Functions for float
void pushFloat(int procID, int& sp, float f) {
    byte* b = (byte*)&f;
    for (int i = 3; i >= 0; i--) {
        // Push float in bytes
        pushByte(procID, sp, b[i]);
    }
    // Push var type
    pushByte(procID, sp, FLOAT);
}
float popFloat(int procID, int& sp) {
    byte b[4];
    for (int i = 0; i < 4; i++) {
        byte temp = popByte(procID, sp);
        b[i] = temp;
    }
    float* f = (float*)b;
    return *f;
}

// Functions for String

void pushString(int procID, int& sp, char* s) {
    for (int i = 0; i < strlen(s); i++) {
        pushByte(procID, sp, s[i]);  // Push string itself
    }
    pushByte(procID, sp, 0x00);           // Send Terminating Zero
    pushByte(procID, sp, strlen(s) + 1);  // Send string lenth + terminating zero
    pushByte(procID, sp, STRING);         // Send varType
}
char* popString(int procID, int& sp, int size) {
    char* temp = new char[size];
    for (int i = size - 1; i >= 0; i--) {
        byte letter = popByte(procID, sp);
        temp[i] = letter;
    }
    return temp;
}

float popVal(int procID, int& sp, int type) {
    switch (type) {
        case 1:  // Char
        {
            return popChar(procID, sp);
            break;
        }
        case 2:  // Int
        {
            return popInt(procID, sp);
            break;
        }
        case 4:  // Float
        {
            return popFloat(procID, sp);
            break;
        }
        default:
            break;
    }
}

// 
// Memory Functions
// 

// Bubble sort the memory
void sortMem() {
    bool sorted = false;
    while (sorted == false) {
        sorted = true;
        for (int i = 0; i < noOfVars - 1; i++) {
            if (memoryTable[i].adress > memoryTable[i + 1].adress) {
                memVar temp = memoryTable[i];
                memoryTable[i] = memoryTable[i + 1];
                memoryTable[i + 1] = temp;
                sorted = false;
            }
        }
    }
}

int findFileInMemory(byte name, int procID) {
    for (int i = 0; i < noOfVars; i++) {
        if (memoryTable[i].name == name && memoryTable[i].procID == procID) {
            return i;
        }
    }
    return -1; 
}

// Check if there is space in the memory and return where the space is
int checkMemSpace(int size) {
    // Check first index
    if (memoryTable[0].adress >= size) {
        return 0;
    }

    // Check between memory entries
    for (int i = 0; i < noOfVars - 1; i++) {
        int availableSpace = memoryTable[i + 1].adress - (memoryTable[i].adress + memoryTable[i].length);
        if (availableSpace >= size) {
            return memoryTable[i].adress + memoryTable[i].length;
        }
    }

    // Check behind last entry
    int lastEntry = memoryTable[noOfVars - 1].adress + memoryTable[noOfVars - 1].length;
    if (MEMORY_MAX_SIZE - (int)lastEntry >= size) {
        return lastEntry;
    }

    Serial.println("No space on RAM available");
    return -1;
}

void addMemoryEntry(byte name, int procID, int &stackP) {
    // 1. Check if there is space available for memEntry
    if (noOfVars >= MEMORY_TABLE_SIZE) {
        Serial.print(F("Error. Not enough space in the memory table"));
        return;
    }

    // 2. Check if name is already taken
    int index = findFileInMemory(name, procID);

    // 3. Delete old memEntry if necessary
    if (index != -1) {
        for (int i = index; i < noOfVars; i++) {
            memoryTable[i] = memoryTable[i + 1];
        }
        noOfVars--;
    }
    index = noOfVars;

    int type = popByte(procID, stackP);
    int size = (type != 3) ? type : popByte(procID, stackP);
    sortMem();

    int newAdress = (noOfVars > 0) ? checkMemSpace(size) : 0;
    memVar newVar = {.name = name,
                            .type = type,
                            .length = size,
                            .adress = newAdress,
                            .procID = procID};

    memoryTable[index] = newVar;

    // Check varType and add to memory with according function
    switch (type) {
        case 1: {       // Char
            saveChar(popVal(procID, stackP, type), newAdress);
            break;
        }
        case 2: {       // Integer
            saveInt(popVal(procID, stackP, type), newAdress);
            break;
        }
        case 3: {       // String
            char *s = popString(procID, stackP, size);
            saveString(s, newAdress);
            break;
        }
        case 4: {       // Float
            saveFloat(popVal(procID, stackP, type), newAdress);
            break;
        }
        default:
            break;
    }

    noOfVars++;
}

void retrieveMemoryEntry(byte name, int procID, int &stackP) {
    // 1. Check if var exists
    int index = findFileInMemory(name, procID);
    if (index == -1) {
        Serial.println("This variable could not be found");
        return;
    }


    // Check vartype and retrieve with according function
    int type = memoryTable[index].type;
    int length = memoryTable[index].length;
    switch (type) {
        case 1: {        // Char
            char temp = loadChar(memoryTable[index].adress);
            pushChar(procID, stackP, temp);
            break;
        }
        case 2: {       // Integer
            int temp = loadInt(memoryTable[index].adress);
            pushInt(procID, stackP, temp);
            break;
        }
        case 3: {       // String
            pushString(procID, stackP, loadString(memoryTable[index].adress, length));
            break;
        }
        case 4: {       // Float
            pushFloat(procID, stackP, loadFloat(memoryTable[index].adress));
            break;
        }
        default:
            break;
    }
}

// Functions for reading/writing memory
// Char
void saveChar(char c, int adress) {
    RAM[adress] = c;
}
char loadChar(int adress) {
    return RAM[adress];
}

// Int
void saveInt(int i, int adress) {
    RAM[adress] = highByte(i);
    RAM[adress + 1] = lowByte(i);
}
int loadInt(int adress) {
    byte hb = RAM[adress];
    byte lb = RAM[adress + 1];
    return word(hb, lb);
}

// String
void saveString(char *s, int adress) {
    for (int i = 0; i < strlen(s); i++) {
        RAM[adress + i] = s[i];
    }
    RAM[adress + strlen(s)] = 0x00;
}
char *loadString(int adress, int length) {
    char *temp = new char[length];
    for (int i = length - 1; i >= 0; i--) {
        temp[i] = RAM[adress + i];
    }
    return temp;
}

// Float
void saveFloat(float f, int adress) {
    byte *b = (byte *)&f;
    for (int i = 3; i >= 0; i--) {
        RAM[adress + i] = b[i];
    }
}
float loadFloat(int adress) {
    byte b[4];
    for (int i = 0; i < 4; i++) {
        b[i] = RAM[adress + i];
    }
    float *f = (float *)b;
    return *f;
}

// Function that will clear all variables for a program when it's terminated
void deleteAllVars(int procID) {
    for (int j = 0; j < noOfVars; j++) {
        if (memoryTable[j].procID == procID) {
            for (int i = j; i < noOfVars; i++) {
                memoryTable[i] = memoryTable[i + 1];
            }
            noOfVars--;
        }
    }
}

//
//  Process Management
//

// Set the state of a program in pt
void setState(int processIndex, char newState) {
    // Check if the correct state argument is given
    if (newState != 'r' && newState != 'p' && newState != '0') {
        Serial.println(F("Not a valid state"));
        return;
    }
    // Check if given state is current state
    if (processTable[processIndex].state == newState) {
        Serial.print(F("Process already is in "));
        Serial.print(newState);
        Serial.println(F(" state"));
        return;
    }
    // Apply state change if no errors in command
    processTable[processIndex].state = newState;
}

void runProcess(const char *filename) {
    // Check if there is enough space on proces table
    if (noOfProc >= PROCESS_TABLE_SIZE) {
        Serial.println(F("Program table is full, kill another program and try again!"));
        return;
    }
    // Check if the program exists in FAT
    int fileIndex = findFATEntry(filename);
    if (fileIndex == -1) {
        Serial.println(F("This file cannot be run as it doesn't exist."));
        return;
    }

    // Create a new process
    process newProcess;
    strcpy(newProcess.name, filename);
    newProcess.procID = processCounter++;
    newProcess.state = 'r';
    newProcess.pc = 0;
    newProcess.fp = 0;
    newProcess.sp = 0;
    newProcess.address = FAT[fileIndex].addr;

    // Increment # of processes
    processTable[noOfProc++] = newProcess;

    Serial.print(F("Started process with PID "));
    Serial.println(newProcess.procID);
}

void suspendProcess(int id) {
    int processIndex = findPid(id);
    // Check if process exists
    if (processIndex == -1) {
        Serial.println(F("This process does not exist."));
        return;
    }
    // Check if process is dead
    if (processTable[processIndex].state == '0') {
        Serial.println(F("You can only suspend running programs."));
        return;
    }
    // Suspend process 
    setState(processIndex, 'p');
    Serial.print(F("Suspend process with PID "));
    Serial.println(id);
}

void resumeProcess(int id) {
    int processIndex = findPid(id);
    // Check if process exists
    if (processIndex == -1) {
        Serial.println(F("This process does not exist."));
        return;
    }
    // Check is process is dead
    if (processTable[processIndex].state == '0') {
        Serial.println(F("This process cannot be resumed as it has been killed"));
        return;
    }

    setState(processIndex, 'r');
    Serial.print(F("Resumed process with PID "));
    Serial.println(id);
}

void killProcess(int id) {
    int processIndex = findPid(id);
    // Check if process is findable
    if (processIndex == -1) {
        Serial.println(F("This process does not exist."));
        return;
    }
    // Check is process is already dead
    if (processTable[processIndex].state == '0') {
        Serial.println(F("This process is already dead"));
        return;
    }
    // Clear vars for process
    deleteAllVars(id);
    // Set state as killed
    setState(processIndex, '0');

    // Remove processtable entry
    for (int j = 0; j < noOfProc; j++) {
        if (processTable[j].procID == id) {
            for (int i = j; i < noOfProc; i++) {
                processTable[i] = processTable[i + 1];
            }
        }
    }
    Serial.print(F("Killed process with PID "));
    Serial.println(id);
    noOfProc--;
}

int findPid(int id)
// Find the index of a process in the process
{
    for (int i = 0; i < noOfProc; i++) {
        if (processTable[i].procID == id) {
            // Return index
            return i;
        }
    }
    return -1;
}

// Show the list of processes with their ID, state, and name
void showProcessList() {
    Serial.println(F("List of active processes:"));

    for (int i = 0; i < noOfProc; i++) {
        if (processTable[i].state != '0') {
            Serial.print(F("ID: "));
            Serial.print(processTable[i].procID);
            Serial.print(F("   Status: "));
            Serial.print(processTable[i].state);
            Serial.print(F("   Name: "));
            Serial.println(processTable[i].name);
        }
    }
}

// Helper function for finding available memory
int freeMem(){
    int size = 0;
    if(noOfProc == 0){ return EEPROM.length()-161; }
    if(noOfProc == 1){ return (EEPROM.length() -161 - memoryTable[0].length ); }
    for(int i = 0; i < noOfProc; i++) {
        if(size < memoryTable[noOfProc+1].adress - (memoryTable[noOfProc].adress + memoryTable[noOfProc].length)){
            size = memoryTable[noOfProc+1].adress - (memoryTable[noOfProc].adress + memoryTable[noOfProc].length);
            return size;
        }
    }
    return -1;
}

//
//  Unary functions
//
float incrementF(int type, float value) {
    return value + 1;
}
float decrementF(int type, float value) {
    return value - 1;
}
float unaryminus(int type, float value) {
    return -value;
}
float logicalNot(int type, float value) {
    return !value;
}
float bitwiseNot(int type, float value) {
    return ~(int)value;
}
float ToChar(int type, float value) {
    return (char)value;
}
float ToInt(int type, float value) {
    return (int)value;
}
float ToFloat(int type, float value) {
    return (float)value;
}
float Round(int type, float value) {
    return round(value);
}
float Floor(int type, float value) {
    return floor(value);
}
float Ceil(int type, float value) {
    return ceil(value);
}
float Abs(int type, float value) {
    return abs(value);
}
float Sq(int type, float value) {
    return value * value;
}
float Sqrt(int type, float value) {
    return sqrt(value);
}
float AnalogRead(int type, float value) {
    int pin = (int)value;
    int rawValue = analogRead(pin);
    return (float)rawValue;

}
float DigitalRead(int type, float value) {
    int pin = (int)value;
    int rawValue = digitalRead(pin);
    return (float)rawValue;
}

typedef struct {
    int operatorName;
    float (*func)(int type, float value);
    int returnType;
} unaryFunction;

unaryFunction unary[] = {
    {INCREMENT, &incrementF, 0},
    {DECREMENT, &decrementF, 0},
    {UNARYMINUS, &unaryminus, 0},
    {LOGICALNOT, &logicalNot, CHAR},
    {BITWISENOT, &bitwiseNot, 0},
    {TOCHAR, &ToChar, CHAR},
    {TOINT, &ToInt, INT},
    {TOFLOAT, &ToFloat, FLOAT},
    {ROUND, &Round, INT},
    {FLOOR, &Floor, INT},
    {CEIL, &Ceil, INT},
    {ABS, &Abs, 0},
    {SQ, &Sq, 0},
    {SQRT, &Sqrt, 0},
    {ANALOGREAD, &AnalogRead, INT},
    {DIGITALREAD, &DigitalRead, CHAR},
};

// find the correct unary function from the array of functions
int findUnaryFunction(int operatorNum) {
    for (int i = 0; i < 2; i++) {
        if (unary[i].operatorName == operatorNum) {
            return i;
        }
    }
    return -1;
}

//
// Binary Functions
//

float plus(float x, float y) {
    return x + y;
}
float minus(float x, float y) {
    return x - y;
}
float times(float x, float y) {
    return x * y;
}
float dividedBy(float x, float y) {
    return x / y;
}
float modulus(float x, float y) {
    return (int)x % (int)y;
}

typedef struct {
    int operatorName;
    float (*func)(float x, float y);
    int returnType;
} binaryFunction;

binaryFunction binary[] = {
    {PLUS, &plus, 0},
    {MINUS, &minus, 0},
    {TIMES, &times, 0},
    {DIVIDEDBY, &dividedBy, 0},
    {MODULUS, &modulus, 0},
};

// Find the correct function from the BinaryFunction array
int findBinaryFunction(int operatorNum) {
    for (int i = 0; i < 2; i++) {
        if (binary[i].operatorName == operatorNum) {
            return i;
        }
    }
    return -1;
}

//
// Executing programs
//

void execute(int index) {
    int address = processTable[index].address;
    int procID = processTable[index].procID;
    int& stackP = processTable[index].sp;
    byte currentCommand = EEPROM.read(address + processTable[index].pc);
    processTable[index].pc++;
    // go through all the bytes and run corresponding program for each command
    switch (currentCommand) {
        
        case CHAR: {
            char temp = (char)EEPROM.read(address + processTable[index].pc++);
            pushChar(procID, stackP, temp);
            break;
        }
        case INT: {
            int highByte = EEPROM.read(address + processTable[index].pc++);
            int lowByte = EEPROM.read(address + processTable[index].pc++);
            pushInt(procID, stackP, word(highByte, lowByte));
            break;
        }
        case STRING: {
            char string[12];
            memset(&string[0], 0, sizeof(string));
            int pointer = 0;
            do {
                int temp = (int)EEPROM.read(address + processTable[index].pc++);
                string[pointer] = (char)temp;
                pointer++;
            } while (string[pointer - 1] != 0);

            pushString(procID, stackP, string);
            break;
        }
        case FLOAT: {
            byte b[4];
            for (int i = 3; i >= 0; i--) {
                byte temp = EEPROM.read(address + processTable[index].pc++);
                b[i] = temp;
            }
            float* f = (float*)b;
            pushFloat(procID, stackP, *f);
            break;
        }
        case STOP: {
            Serial.print(F("Process with pid: "));
            Serial.print(procID);
            Serial.println(F(" is finished."));
            deleteAllVars(procID);
            killProcess(procID);
            Serial.println();
            break;
        }
        case 7 ... 8: {
            // Handle unary functions
            int type = popByte(procID, stackP);
            float value = popVal(procID, stackP, type);

            float newValue = unary[findUnaryFunction(currentCommand)].func(type, value);

            switch (type) {
                case CHAR: {
                    pushChar(procID, stackP, (char)newValue);
                    break;
                }
                case INT: {
                    pushInt(procID, stackP, (int)newValue);
                    break;
                }
                case FLOAT: {
                    pushFloat(procID, stackP, (float)newValue);
                    break;
                }
                default:
                    Serial.println(F("Execute: Default case"));
                    break;
            }
            break;
        }
        case 9 ... 10: {
            int typeY = popByte(procID, stackP);
            float y = popVal(procID, stackP, typeY);
            int typeX = popByte(procID, stackP);
            float x = popVal(procID, stackP, typeY);

            float newValue = binary[findBinaryFunction(currentCommand)].func(x, y);
            int returnType = max(typeY, typeX);
            switch (returnType) {
                case CHAR: {
                    pushChar(procID, stackP, (char)newValue);
                    break;
                }
                case INT: {
                    pushInt(procID, stackP, (int)newValue);
                    break;
                }
                case FLOAT: {
                    pushFloat(procID, stackP, (float)newValue);
                    break;
                }
                default:
                    Serial.println(F("Execute: Default case"));
                    break;
            }
            break;
        }
        default: {
            Serial.println(F("Error. Unkown commandList."));
            break;
        }
        case 51 ... 52: {  // Print commands
            int type = popByte(procID, stackP);
            switch (type) {
                case CHAR: {
                    Serial.print(popChar(procID, stackP));
                    break;
                }
                case INT: {
                    Serial.print(popInt(procID, stackP));
                    break;
                }
                case STRING: {
                    int size = popByte(procID, stackP);
                    Serial.print(popString(procID, stackP, size));
                    break;
                }
                case FLOAT: {
                    Serial.print(popFloat(procID, stackP), 5);
                    break;
                }
                default:
                    break;
            }
            if (currentCommand == 52) {
                Serial.println();
            }
            break;
        }
        case SET: {
            char name = EEPROM.read(address + processTable[index].pc++);
            addMemoryEntry(name, procID, stackP);
            break;
        }
        case GET: {
            char name = EEPROM.read(address + processTable[index].pc++);
            retrieveMemoryEntry(name, procID, stackP);
            break;
        }
        case DELAY: {
            break;
        }
        case DELAYUNTIL: {
            popByte(procID, stackP);
            int temp = popInt(procID, stackP);
            int mil = millis();
            if (temp > mil) {
                processTable[index].pc--;
                pushInt(procID, stackP, temp);
            }
            break;
        }
        case MILLIS: {
            pushInt(procID, stackP, millis());
            break;
        }
        case PINMODE: {
            popByte(procID, stackP);
            int direction = popInt(procID, stackP);
            popByte(procID, stackP);
            int pin = popInt(procID, stackP);
            pinMode(pin, direction);
            break;
        }
        case DIGITALWRITE: {
            popByte(procID, stackP);
            int status = popInt(procID, stackP);
            popByte(procID, stackP);
            int pin = popInt(procID, stackP);
            digitalWrite(pin, status);
            break;
        }
        case FORK: {
            int type = popByte(procID, stackP);
            int size = popByte(procID, stackP);
            char* fileName = popString(procID, stackP, size);
            runProcess(fileName);
            pushInt(procID,stackP,procID+1);
            break;
        }
        case WAITUNTILDONE: {
            popByte(procID,stackP);
            int runningID = popInt(procID, stackP);
            char state = processTable[runningID].state;
            if(state == 'r' || state == 'p') {
                processTable[procID].pc--;
                pushInt(procID, stackP, runningID);
            } 
            break;
        }
    }
}

void runProcesses() {
    for (int i = 0; i < noOfProc; i++) {
        if (processTable[i].state == 'r') {
            execute(i);
        }
    }
}

//
// Main functions
//

void store() {
    // Store a file in file system
    storeFile(CLIBuffer[1], atoi(CLIBuffer[2]));
}
void retrieve() {
    // Retrieve a file from file system
    retrieveFile(CLIBuffer[1]);
}
void erase() {
    // Delete a file
    eraseFile(CLIBuffer[1]);
}
void files() {
    // Print a list of files
    ls();
}
void freespace() {
    // Print the available space in file system
    df();
}
void run() {
    // Start a program
    runProcess(CLIBuffer[1]);
}
void list() {
    showProcessList();
}
void suspend() {
    // Suspend a process
    if (isNumeric()) {
        suspendProcess(atoi(CLIBuffer[1]));
    } else {
        Serial.println(F("Error. Invalid process ID."));
    }
}
void resume() {
    // Resume a process
    if (isNumeric()) {
        resumeProcess(atoi(CLIBuffer[1]));
    } else {
        Serial.println(F("Error. Invalid process ID."));
    }
}
void kill() {
    // Stop a process
    if (isNumeric()) {
        killProcess(atoi(CLIBuffer[1]));
    } else {
        Serial.println(F("Error. Invalid process ID."));
    }
}
void clear() {
    for (int i = 0; i < 10; i++){
        Serial.println("\n");
    }
}

//
// Arduino Functions
//
void setup() {
    Serial.begin(9600);
    Serial.println(F("\nArduinOS 2.0 ready.\n"));
    // clearEeprom();
}

void loop() {
    commandLine();
    runProcesses();
}