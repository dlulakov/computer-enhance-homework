#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BUFFER_SIZE 1024

typedef enum move_instruction {
  REG_TO_REG,
  IMMEDIATE_TO_REG_MEMORY,
  IMMEDIATE_TO_REG,
  MEMORY_TO_ACCUMULATOR,
  ACCUMULATOR_TO_MEMORY
} MOVE_INSTRUCTION;

struct RegBits {
  char reg[3];
  char bits[4];
};

struct Instruction {
  MOVE_INSTRUCTION move_instruction;
  int w;

  union {
    struct {
      char reg[3];
      char rm[3];
      char mod[3];
      int d;
    } reg_to_reg;

    struct {
      char reg[3];
      int data;
    } immediate_to_reg;
  } type_of_instruction;
};

static const char *move_instructions[] = {"100010", "1100011", "1011",
                                          "1010000", "1010001"};

static const char *bits[] = {"000", "001", "010", "011",
                             "100", "101", "110", "111"};
static const char *reg_full_w1[] = {"ax", "cx", "dx", "bx",
                                    "sp", "bp", "si", "di"};
static const char *reg_full_w0[] = {"al", "cl", "dl", "bl",
                                    "ah", "ch", "dh", "bh"};

void printInstruction(struct Instruction instruction, FILE *fptr_writing) {

  switch (instruction.move_instruction) {
  case REG_TO_REG:
    printf("\n ------ Printing Instruction Reg To Reg ------ \n");
    printf("Instruction D: %d\n", instruction.type_of_instruction.reg_to_reg.d);
    printf("Instruction W: %d\n", instruction.w);
    printf("Instruction MOD: %s\n",
           instruction.type_of_instruction.reg_to_reg.mod);
    printf("Instruction REG: %s\n",
           instruction.type_of_instruction.reg_to_reg.reg);
    printf("Instruction RM: %s\n",
           instruction.type_of_instruction.reg_to_reg.rm);
    if (instruction.type_of_instruction.reg_to_reg.d)
      printf("mov %s, %s\n", instruction.type_of_instruction.reg_to_reg.reg,
             instruction.type_of_instruction.reg_to_reg.rm);
    else
      printf("mov %s, %s\n", instruction.type_of_instruction.reg_to_reg.rm,
             instruction.type_of_instruction.reg_to_reg.reg);
    printf(" ---- Ending the Printing of Instruction -----  \n");
    if (instruction.type_of_instruction.reg_to_reg.d)
      fprintf(fptr_writing, "mov %s, %s\n",
              instruction.type_of_instruction.reg_to_reg.reg,
              instruction.type_of_instruction.reg_to_reg.rm);
    else
      fprintf(fptr_writing, "mov %s, %s\n",
              instruction.type_of_instruction.reg_to_reg.rm,
              instruction.type_of_instruction.reg_to_reg.reg);
    break;
  case IMMEDIATE_TO_REG:
    printf("\n ------ Printing Instruction Immediate to Reg ------ \n");
    printf("Instruction W: %d\n", instruction.w);
    printf("Instruction REG: %s\n",
           instruction.type_of_instruction.immediate_to_reg.reg);
    printf("Instruction data: %d\n",
           instruction.type_of_instruction.immediate_to_reg.data);
    printf("mov %s, %d\n", instruction.type_of_instruction.immediate_to_reg.reg,
           instruction.type_of_instruction.immediate_to_reg.data);
    printf(" ---- Ending the Printing of Instruction -----  \n");

    fprintf(fptr_writing, "mov %s, %d\n",
            instruction.type_of_instruction.immediate_to_reg.reg,
            instruction.type_of_instruction.immediate_to_reg.data);
    break;
  default:
    printf("Instruction not Supported yet");
    break;
  }
}

void printParts(int *bit_array) {
  printf("\n");
  for (int i = 0; i < sizeof(bit_array); ++i) {
    printf("bit_array[%d] = %d\n", i, bit_array[i]);
  }
}

int getDBit(int *binary_array) { return binary_array[6]; }
int getWBit(int *binary_array, bool immediate_to_register) {
  if (immediate_to_register) {
    return binary_array[4];
  }
  return binary_array[7];
}

void getBinaryElementsForImmediateToRegister(int *REG, int *data,
                                             int *binary_array, int W) {
  int reg_counter = 0;
  int data_counter = 0;
  for (int i = 0; i < 24; ++i) {
    if (i > 4 && i <= 7) {
      REG[reg_counter] = binary_array[i];
      reg_counter++;
    } else if (i > 7 && i <= 15) {
      data[data_counter] = binary_array[i];
      data_counter++;
    } else if (W == 1 && i > 15 && i <= 23) {
      data[data_counter] = binary_array[i];
      data_counter++;
    }
  }
}

void getBinaryElementsForRegToReg(int *MOD, int *REG, int *RM,
                                  int *binary_array) {
  int reg_counter = 0;
  int mod_counter = 0;
  int rm_counter = 0;
  for (int i = 0; i < 16; ++i) {
    if (i > 7 && i <= 9) {
      MOD[mod_counter] = binary_array[i];
      mod_counter++;
    } else if (i > 9 && i <= 12) {
      REG[reg_counter] = binary_array[i];
      reg_counter++;
    } else if (i > 12) {
      RM[rm_counter] = binary_array[i];
      rm_counter++;
    }
  }
}

MOVE_INSTRUCTION getTypeOfMoveInstruction(int *bit_array) {
  if (bit_array[3]) {
    return IMMEDIATE_TO_REG;
  } else if (bit_array[1]) {
    return IMMEDIATE_TO_REG_MEMORY;
  } else if (bit_array[2]) {
    // Accumulator
    if (bit_array[6]) {
      return ACCUMULATOR_TO_MEMORY;
    } else {
      return MEMORY_TO_ACCUMULATOR;
    }
  } else {
    return REG_TO_REG;
  }
}

struct Instruction createMoveInstructionImmediateToReg(int W, int *REG,
                                                       int *data) {
  struct Instruction instruction;
  instruction.move_instruction = IMMEDIATE_TO_REG;

  // REG
  char reg[4];
  for (int i = 0; i < 3; ++i) {
    char c = (char)(REG[i] + '0');
    reg[i] = c;
  }
  reg[3] = '\0';

  printf("\nW: %d\n", W);

  int number = 0;
  int power = 0;
  bool negative_number = false;
  if (W == 1) {
    instruction.w = 1;
    // Get REG
    for (int j = 0; j < 8; ++j) {
      if (strcmp(reg, bits[j]) == 0) {
        strcpy(instruction.type_of_instruction.immediate_to_reg.reg,
               reg_full_w1[j]);
      }
    }
    // Get data
    for (int j = 15; j != 0; --j) {
      number = number + (data[j] * pow(2, power));
      power++;
    }

  } else {
    instruction.w = 0;
    // Get Reg Bit
    for (int j = 0; j < 8; ++j) {
      if (strcmp(reg, bits[j]) == 0) {
        strcpy(instruction.type_of_instruction.immediate_to_reg.reg,
               reg_full_w0[j]);
      }
    }

    // Handle when the number is negative
    if (data[0] == 1) {
      // Transform every 1 -> 0 and 0 -> 1
      for (int i = 0; i < 8; ++i) {
        data[i] = data[i] == 0 ? 1 : 0;
      }

      negative_number = true;
    }

    // Get data
    for (int j = 7; j != 0; --j) {
      number = number + (data[j] * pow(2, power));
      power++;
    }
  };

  printf("Number :%d", number);
  instruction.type_of_instruction.immediate_to_reg.data =
      negative_number ? -(++number) : number;
  return instruction;
}

struct Instruction createMoveInstructionRegToReg(int D, int W, int *MOD,
                                                 int *REG, int *RM) {
  struct Instruction instruction;
  instruction.move_instruction = REG_TO_REG;
  instruction.type_of_instruction.reg_to_reg.d = D;

  // MOD
  char mod[3];
  for (int i = 0; i < 2; ++i) {
    char c = (char)(MOD[i] + '0');
    mod[i] = c;
  }
  mod[2] = '\0';

  strcpy(instruction.type_of_instruction.reg_to_reg.mod, mod);

  // REG
  char reg[4];
  for (int i = 0; i < 3; ++i) {
    char c = (char)(REG[i] + '0');
    reg[i] = c;
  }
  reg[3] = '\0';

  // R/M
  char rm[4];
  for (int i = 0; i < 3; ++i) {
    char c = (char)(RM[i] + '0');
    rm[i] = c;
  }
  rm[3] = '\0';

  printf("\nW: %d\n", W);
  if (W == 1) {
    instruction.w = 1;
    for (int j = 0; j < 8; ++j) {
      if (strcmp(reg, bits[j]) == 0) {
        strcpy(instruction.type_of_instruction.reg_to_reg.reg, reg_full_w1[j]);
      }
      if (strcmp(rm, bits[j]) == 0) {
        strcpy(instruction.type_of_instruction.reg_to_reg.rm, reg_full_w1[j]);
      }
    }
  } else {
    instruction.w = 0;
    for (int j = 0; j < 8; ++j) {
      if (strcmp(reg, bits[j]) == 0) {
        strcpy(instruction.type_of_instruction.reg_to_reg.reg, reg_full_w0[j]);
      }
      if (strcmp(rm, bits[j]) == 0) {
        strcpy(instruction.type_of_instruction.reg_to_reg.rm, reg_full_w0[j]);
      }
    }
  };

  return instruction;
}

void decToBinary(int *binary_array, int n, int step) {
  int binaryNum[1000];
  int i = 0;
  while (n > 0) {
    binaryNum[i] = n % 2;
    n = n / 2;
    i++;
  }

  // Handle bits that have 0 as the most significant bit
  int left_over = 8 - i;
  int begin = 0;
  while (left_over > 0) {
    printf("%d", 0);
    binary_array[begin] = 0;
    begin++;
    left_over--;
  }

  for (int j = i - 1, k = 0; j >= 0; j--, k++) {
    printf("%d", binaryNum[j]);
    binary_array[begin + k + (step * 8)] = binaryNum[j];
  }

  printf(" ");
}

int main(int argc, char *argv[]) {
  FILE *fptr;
  unsigned char buffer[BUFFER_SIZE];
  size_t bytes_read;
  int binary_array[1000];

  if (argc > 1) {
    fptr = fopen(argv[1], "rb");
  } else {
    printf("Please add the execution file\n");
    return 0;
  }

  if (fptr == NULL) {
    printf("Error: Could not open the file");
    return 1;
  }

  uint8_t bytes[6];
  FILE *fptr_writing = fopen("result.asm", "w");
  fprintf(fptr_writing, "bits 16\n");
  while (fread(bytes, 1, 2, fptr) == 2) {
    int binary_array[24];

    printf("\nPrinting the Binary: \n");
    decToBinary(binary_array, bytes[0], 0);
    decToBinary(binary_array + 8, bytes[1], 0);
    MOVE_INSTRUCTION move_instruction = getTypeOfMoveInstruction(binary_array);
    int W;
    int REG[3];
    switch (move_instruction) {
    case REG_TO_REG:
      printf("\nREG_TO_REG\n");
      int D = getDBit(binary_array);
      W = getWBit(binary_array, false);
      int MOD[2];
      int RM[3];
      getBinaryElementsForRegToReg(MOD, REG, RM, binary_array);
      struct Instruction instructionRegToReg =
          createMoveInstructionRegToReg(D, W, MOD, REG, RM);

      printInstruction(instructionRegToReg, fptr_writing);
      break;
    case IMMEDIATE_TO_REG:
      printf("\nIMMEDIATE_TO_REG\n");
      uint8_t bytesI[1];
      int data[16];
      W = getWBit(binary_array, true);
      if (W == 1 && fread(bytesI, 1, 1, fptr) == 1) {
        decToBinary(binary_array + 16, bytesI[0], 0);
      }
      getBinaryElementsForImmediateToRegister(REG, data, binary_array, W);

      struct Instruction instructionImmediateToReg =
          createMoveInstructionImmediateToReg(W, REG, data);

      printInstruction(instructionImmediateToReg, fptr_writing);
      break;
    case IMMEDIATE_TO_REG_MEMORY:
      printf("\nIMMEDIATE_TO_REG_MEMORY\n");
      W = getWBit(binary_array, false);
      break;
    case ACCUMULATOR_TO_MEMORY:
      printf("\nACCUMULATOR_TO_MEMORY\n");
      W = getWBit(binary_array, false);
      break;
    case MEMORY_TO_ACCUMULATOR:
      printf("\nMEMORY_TO_ACCUMULATOR\n");
      W = getWBit(binary_array, false);
      break;
    }

    // printParts(D, W, MOD, REG, RM);
  }
  fclose(fptr_writing);
  fclose(fptr);
  return 0;
}
