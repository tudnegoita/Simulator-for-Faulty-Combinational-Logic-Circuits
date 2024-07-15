#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_ROWS 10
#define MAX_COLS 10
#define MAX_FAULTS 100
#define MAX_TEST_VECTORS 100

struct gate {
    char type;
    int ni;
    int valFN;
    int valCD;
    int inputs[4][2];
    int stuckAtFault;
};

struct fault {
    int row;
    int col;
    int type;
};

struct gate CLC[MAX_ROWS][MAX_COLS];
int outputGates[MAX_ROWS][2];
int numOutputs = 0;
char primaryInputs[MAX_COLS];
int numPrimaryInputs;
struct fault faults[MAX_FAULTS];
int numFaults = 0;
char testVectors[MAX_TEST_VECTORS][MAX_COLS];
int testVectorFaults[MAX_TEST_VECTORS][MAX_FAULTS];
int numTestVectors = 0;

void setGate(int row, int col, char type, int ni) {
    CLC[row][col].type = type;
    CLC[row][col].ni = ni;
    CLC[row][col].valFN = -1;
    CLC[row][col].valCD = -1;
    CLC[row][col].stuckAtFault = -1;
}

void readCLC(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    char line[100];
    while (fgets(line, sizeof(line), file)) {
        char type;
        int row, col, ni;

        if (sscanf(line, "Out %d,%d", &row, &col) == 2) {
            row -= 1; col -= 1;
            outputGates[numOutputs][0] = row;
            outputGates[numOutputs][1] = col;
            numOutputs++;
        }
        else if (sscanf(line, "%c %d,%d %d", &type, &row, &col, &ni) == 4) {
            row -= 1; col -= 1;
            setGate(row, col, type, ni);

            char* inputs = strchr(line, ' ') + 1;
            inputs = strchr(inputs, ' ') + 1;
            inputs = strchr(inputs, ' ') + 1;

            for (int i = 0; i < ni; i++) {
                int inputRow, inputCol;
                if (sscanf(inputs, "%d,%d", &inputRow, &inputCol) == 2) {
                    inputRow -= 1; inputCol -= 1;
                    if (inputRow < row) {
                        CLC[row][col].inputs[i][0] = inputRow;
                        CLC[row][col].inputs[i][1] = inputCol;
                    }
                    else {
                        printf("Error: Gate at (%d, %d) has an invalid input from (%d, %d)\n", row + 1, col + 1, inputRow + 1, inputCol + 1);
                        fclose(file);
                        return;
                    }
                    inputs = strchr(inputs, ' ');
                    if (inputs) inputs++;
                }
            }
        }
        else if (sscanf(line, "In %d,%d", &row, &col) == 2) {
            numPrimaryInputs++;
            row -= 1;
            col -= 1;
            setGate(row, col, 'I', 0);
        }
    }
    fclose(file);
}

void promptForPrimaryInputs(char inputValues[MAX_COLS]) {
    printf("Enter the values for the primary inputs (e.g., 110 for 3 inputs). Enter -1 to stop.\n");
    printf("Primary inputs: ");
    scanf("%s", inputValues);
}

void resetValFN() {
    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            CLC[row][col].valFN = -1;
        }
    }
}

void applyPrimaryInputs(const char* inputValues) {
    resetValFN();
    int length = strlen(inputValues);
    for (int i = 0; i < length; i++) {
        int row = 0;
        int col = i;

        if (CLC[row][col].type == 'I') {
            CLC[row][col].valFN = inputValues[i] - '0';
        }
        else {
            printf("Error: No input gate at (%d, %d)\n", row + 1, col + 1);
        }
    }
}

void resetValCD() {
    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {

            CLC[row][col].valCD = -1;
            CLC[row][col].stuckAtFault = -1;
        }
    }
}



/*void printDetailedState() {
    printf("Detailed state of all gates:\n");
    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            if (CLC[row][col].type != 0) {
                printf("Gate at (%d, %d): Type = %c, valFN = %d, valCD = %d, Fault = %d\n",
                    row + 1, col + 1, CLC[row][col].type, CLC[row][col].valFN,
                    CLC[row][col].valCD, CLC[row][col].stuckAtFault);
            }
        }
    }
}*/

void printOutputWithFaults() {
    printf("Circuit outputs with the current fault:\n");
    for (int i = 0; i < numOutputs; i++) {
        int row = outputGates[i][0];
        int col = outputGates[i][1];
        int outputValue = evaluateGate(&CLC[row][col], 1);
        printf("Output at (%d, %d): %d\n", row + 1, col + 1, outputValue);
        if (outputValue != evaluateGate(&CLC[row][col], 0)) {
            printf("Fault detected\n");
        }
        else {
            printf("Fault not detected\n");
        }
    }

    /*printDetailedState();*/
}

void promptForFaults() {

    printf("Enter faults in the format 'row,col fault_type' (e.g., '3,2 s0' for stuck-at-0, '3,2 s1' for stuck-at-1). Enter -1 to stop.\n");
    while (1) {
        resetValCD();
        printf("Insert fault: ");
        char faultInput[20];
        scanf(" %[^\n]%*c", faultInput);

        if (strcmp(faultInput, "-1") == 0) break;

        int row, col;
        char faultType[3];
        if (sscanf(faultInput, "%d,%d %s", &row, &col, faultType) == 3) {
            row -= 1;
            col -= 1;

            if (CLC[row][col].type != 0 && (strcmp(faultType, "s0") == 0 || strcmp(faultType, "s1") == 0)) {
                if (strcmp(faultType, "s0") == 0) {
                    CLC[row][col].stuckAtFault = 0;
                }
                else if (strcmp(faultType, "s1") == 0) {
                    CLC[row][col].stuckAtFault = 1;
                }

                printf("Evaluating circuit with fault: %s...\n", faultInput);
                for (int i = 0; i < numOutputs; i++) {
                    int row = outputGates[i][0];
                    int col = outputGates[i][1];
                    evaluateGate(&CLC[row][col], 1);
                }

                printOutputWithFaults();

                resetValCD();
                applyPrimaryInputs(primaryInputs);
            }
            else {
                printf("Error: No gate at (%d, %d) or invalid fault type\n", row + 1, col + 1);
            }
        }
        else {
            printf("Invalid input format.\n");
        }
    }
}

int logicAND(struct gate* g, int useFault) {
    for (int i = 0; i < g->ni; i++) {
        int inputRow = g->inputs[i][0];
        int inputCol = g->inputs[i][1];
        int inputVal = evaluateGate(&CLC[inputRow][inputCol], useFault);
        if (inputVal == 0) {
            return 0;
        }
    }
    return 1;
}

int logicOR(struct gate* g, int useFault) {
    for (int i = 0; i < g->ni; i++) {
        int inputRow = g->inputs[i][0];
        int inputCol = g->inputs[i][1];
        int inputVal = evaluateGate(&CLC[inputRow][inputCol], useFault);
        if (inputVal == 1) {
            return 1;
        }
    }
    return 0;
}

int logicNOT(struct gate* g, int useFault) {
    if (g->ni != 1) return -1;
    int inputRow = g->inputs[0][0];
    int inputCol = g->inputs[0][1];
    return !evaluateGate(&CLC[inputRow][inputCol], useFault);
}

int logicXOR(struct gate* g, int useFault) {
    int result = 0;
    for (int i = 0; i < g->ni; i++) {
        int inputRow = g->inputs[i][0];
        int inputCol = g->inputs[i][1];
        result ^= evaluateGate(&CLC[inputRow][inputCol], useFault);
    }
    return result;
}

int splitGate(struct gate* g, int useFault) {
    if (g->ni != 1) return -1;
    int inputRow = g->inputs[0][0];
    int inputCol = g->inputs[0][1];
    return evaluateGate(&CLC[inputRow][inputCol], useFault);
}

int evaluateGate(struct gate* g, int useFault) {
    if (useFault) {
        if (g->valCD != -1) return g->valCD;

        int result;
        switch (g->type) {
        case 'a': result = logicAND(g, useFault); break;
        case 'o': result = logicOR(g, useFault); break;
        case 'n': result = logicNOT(g, useFault); break;
        case 'x': result = logicXOR(g, useFault); break;
        case 'A': result = !logicAND(g, useFault); break;
        case 'O': result = !logicOR(g, useFault); break;
        case 'X': result = !logicXOR(g, useFault); break;
        case 's': result = splitGate(g, useFault); break;
        case 'I': result = g->valFN; break;
        default: result = -1; break;
        }

        if (g->stuckAtFault != -1) {
            g->valCD = g->stuckAtFault;
        }
        else {
            g->valCD = result;
        }
        return g->valCD;
    }
    else {
        if (g->valFN != -1) return g->valFN;

        int result;
        switch (g->type) {
        case 'a': result = logicAND(g, useFault); break;
        case 'o': result = logicOR(g, useFault); break;
        case 'n': result = logicNOT(g, useFault); break;
        case 'x': result = logicXOR(g, useFault); break;
        case 'A': result = !logicAND(g, useFault); break;
        case 'O': result = !logicOR(g, useFault); break;
        case 'X': result = !logicXOR(g, useFault); break;
        case 's': result = splitGate(g, useFault); break;
        case 'I': result = g->valFN; break;
        default: result = -1; break;
        }

        g->valFN = result;
        return result;
    }
}

void generateTruthTable(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    int numPrimaryInputs = 0;
    for (int i = 0; i < MAX_COLS; i++) {
        if (CLC[0][i].type == 'I') {
            numPrimaryInputs++;
        }
    }

    int numCombinations = pow(2, numPrimaryInputs);

    for (int i = 1; i <= numPrimaryInputs; i++) {
        fprintf(file, "x%d ", i);
    }
    fprintf(file, "  ");
    for (int i = 1; i <= numOutputs; i++) {
        fprintf(file, "z%d ", i);
    }
    fprintf(file, "\n");

    for (int combination = 0; combination < numCombinations; combination++) {
        char inputValues[MAX_COLS];
        for (int i = 0; i < numPrimaryInputs; i++) {
            inputValues[i] = ((combination >> (numPrimaryInputs - i - 1)) & 1) + '0';
        }
        inputValues[numPrimaryInputs] = '\0';

        resetValCD();
        applyPrimaryInputs(inputValues);

        for (int i = 0; i < numPrimaryInputs; i++) {
            fprintf(file, "%c  ", inputValues[i]);
        }
        fprintf(file, "  ");

        for (int i = 0; i < numOutputs; i++) {
            int row = outputGates[i][0];
            int col = outputGates[i][1];
            int outputValue = evaluateGate(&CLC[row][col], 0);
            fprintf(file, "%d  ", outputValue);
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

void generateFaultyTruthTable(const char* filename, int faultRow, int faultCol, int faultType) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    int numPrimaryInputs = 0;
    for (int i = 0; i < MAX_COLS; i++) {
        if (CLC[0][i].type == 'I') {
            numPrimaryInputs++;
        }
    }

    int numCombinations = pow(2, numPrimaryInputs);

    for (int i = 1; i <= numPrimaryInputs; i++) {
        fprintf(file, "x%d ", i);
    }
    fprintf(file, "  ");
    for (int i = 1; i <= numOutputs; i++) {
        fprintf(file, "z%d ", i);
    }
    fprintf(file, "\n");

    for (int combination = 0; combination < numCombinations; combination++) {
        char inputValues[MAX_COLS];
        for (int i = 0; i < numPrimaryInputs; i++) {
            inputValues[i] = ((combination >> (numPrimaryInputs - i - 1)) & 1) + '0';
        }
        inputValues[numPrimaryInputs] = '\0';

        resetValCD();
        applyPrimaryInputs(inputValues);

        CLC[faultRow][faultCol].stuckAtFault = faultType;

        for (int i = 0; i < numPrimaryInputs; i++) {
            fprintf(file, "%c  ", inputValues[i]);
        }
        fprintf(file, "  ");

        for (int i = 0; i < numOutputs; i++) {
            int row = outputGates[i][0];
            int col = outputGates[i][1];
            int outputValue = evaluateGate(&CLC[row][col], 1);
            if (outputValue == evaluateGate(&CLC[row][col], 0)) {
                fprintf(file, "%d  ", outputValue);
            }
            else {
                fprintf(file, "%d*  ", outputValue);
            }
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

void findAllFaults() {
    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            if (CLC[row][col].type != 0) {
                faults[numFaults].row = row;
                faults[numFaults].col = col;
                faults[numFaults].type = 0;
                numFaults++;
                faults[numFaults].row = row;
                faults[numFaults].col = col;
                faults[numFaults].type = 1;
                numFaults++;
            }
        }
    }
}

void generateRandomTestVector(char* testVector) {
    for (int i = 0; i < numPrimaryInputs; i++) {
        testVector[i] = '0' + rand() % 2;
    }
    testVector[numPrimaryInputs] = '\0';
}

int isFaultDetected(struct fault* f, const char* testVector) {
    resetValCD();
    applyPrimaryInputs(testVector);
    int row = f->row;
    int col = f->col;
    CLC[row][col].stuckAtFault = f->type;

    for (int i = 0; i < numOutputs; i++) {
        int outRow = outputGates[i][0];
        int outCol = outputGates[i][1];
        int outputWithFault = evaluateGate(&CLC[outRow][outCol], 1);
        int outputWithoutFault = evaluateGate(&CLC[outRow][outCol], 0);

        if (outputWithFault != outputWithoutFault) {
            return 1;
        }
    }
    return 0;
}

void detectAllFaults() {
    int detectedFaults[MAX_FAULTS] = { 0 };
    int remainingFaults = numFaults;

    srand(time(0));

    while (remainingFaults > 0) {
        char testVector[MAX_COLS];
        generateRandomTestVector(testVector);

        int faultsDetectedInThisRun = 0;
        int detectedFaultIndices[MAX_FAULTS];
        int detectedFaultIndexCount = 0;

        for (int i = 0; i < numFaults; i++) {
            if (!detectedFaults[i]) {
                if (isFaultDetected(&faults[i], testVector)) {
                    detectedFaults[i] = 1;
                    detectedFaultIndices[detectedFaultIndexCount++] = i;
                    faultsDetectedInThisRun++;
                }
            }
        }

        if (faultsDetectedInThisRun > 0) {
            strcpy(testVectors[numTestVectors], testVector);
            numTestVectors++;
            remainingFaults -= faultsDetectedInThisRun;

            for (int i = 0; i < detectedFaultIndexCount; i++) {
                detectedFaults[detectedFaultIndices[i]] = 1;
            }
        }
    }

    printf("All faults have been detected. Test vectors and detected faults:\n");
    for (int i = 0; i < numTestVectors; i++) {
        printf("Test Vector: %s\n", testVectors[i]);
        printf("Detected Faults: ");
        for (int j = 0; j < numFaults; j++) {
            if (isFaultDetected(&faults[j], testVectors[i])) {
                printf("gate %d,%d s%d; ", faults[j].row + 1, faults[j].col + 1, faults[j].type);
            }
        }
        printf("\n");
    }
}



int main() {
    char filename[100];
    printf("Enter the name of the file to be read: ");
    scanf("%s", filename);

    readCLC(filename);

    int choice;
    do {
        printf("Select an option:\n");
        printf("1. Generate truth table without fault\n");
        printf("2. Generate truth table with a specified fault\n");
        printf("3. Insert faults one by one for certain inputs\n");
        printf("4. Generate a test vectors set that detect all faults\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
        case 1:
            generateTruthTable("truth_table_no_fault.txt");
            printf("Truth table without fault generated in 'truth_table_no_fault.txt'.\n");
            break;

        case 2: {
            int faultRow, faultCol, faultType;
            char faultTypeStr[3];
            printf("Enter the fault in the format 'row,col fault_type' (e.g., '3,2 s0' for stuck-at-0, '3,2 s1' for stuck-at-1): ");
            scanf("%d,%d %s", &faultRow, &faultCol, faultTypeStr);
            faultRow -= 1; faultCol -= 1;
            if (strcmp(faultTypeStr, "s0") == 0) {
                faultType = 0;
            }
            else if (strcmp(faultTypeStr, "s1") == 0) {
                faultType = 1;
            }
            else {
                printf("Invalid fault type. Please enter 's0' or 's1'.\n");
                break;
            }
            char faultyFilename[100];
            printf("Enter the filename for the faulty truth table: ");
            scanf("%s", faultyFilename);
            generateFaultyTruthTable(faultyFilename, faultRow, faultCol, faultType);
            printf("Faulty truth table generated in '%s'.\n", faultyFilename);
            break;
        }

        case 3:
            promptForPrimaryInputs(primaryInputs);
            while (strcmp(primaryInputs, "-1") != 0) {
                applyPrimaryInputs(primaryInputs);

                printf("Evaluating circuit without faults...\n");
                for (int i = 0; i < numOutputs; i++) {
                    int row = outputGates[i][0];
                    int col = outputGates[i][1];
                    evaluateGate(&CLC[row][col], 0);
                }

                printOutputWithFaults();

                promptForFaults();

                printf("Enter the values for the primary inputs (e.g., 110 for 3 inputs). Enter -1 to stop.\n");
                printf("Primary inputs: ");
                scanf("%s", primaryInputs);
            }
            break;

        case 4:
            findAllFaults();
            detectAllFaults();
            break;

        case 5:
            printf("Exiting...");
            break;

        default:
            printf("Invalid choice. Please enter a number between 1 and 5.\n");
            break;
        }
    } while (choice != 5);

    return 0;
}


