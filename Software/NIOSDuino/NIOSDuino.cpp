
#include <Arduino.h>
#include <SPIFlash.h>

SPIFlash flash(31);

void eraseFlash();
void saveIntegerFlash(uint32_t addr, int data);
int readIntegerFlash(uint32_t addr);
int readArray(int* array);
void configNN();
void loadNN();
int readInt();

void setup() {
    Serial0.begin(115200);
    
    if (flash.initialize()) Serial0.println("Init OK!");
    else Serial0.println("Init FAIL!");
}

void loop() {
    configNN();
}

void loadNN(){
    Serial0.print("Load Neural Net from Flash...");
}

void configNN (){
    Serial0.print("Number of total layers: ");
    int layerNum = readInt();
    Serial0.println(layerNum);
    int layerSize[layerNum];
    for (int i = 0; i < layerNum; i ++)
    {
        Serial0.print("Neurons in layer ");
        Serial0.print(i+1);
        Serial0.print(": ");
        layerSize[i] = readInt();
        Serial0.println(layerSize[i]);
    }
    int allWeightsSize = 0;
    for (int i = 1; i < layerNum; i ++){
        allWeightsSize += layerSize[i-1] * layerSize[i];
    }
    Serial0.print("Number of all weights: ");
    Serial0.println(allWeightsSize);
    int allWeights[allWeightsSize];
    int allWeightsAddr = 0;
    for (int i = 1; i < layerNum; i ++)
    {
        for (int j = 0; j < layerSize[i]; j++) {
            Serial0.print("Weights for Neuron ");
            Serial0.print(j+1);
            Serial0.print(" in layer ");
            Serial0.print(i+1);
            Serial0.print(" for neuron 1 to ");
            Serial0.print(layerSize[i-1]);
            Serial0.print(" of previous layer: ");
            int weights[layerSize[i-1]];
            int rec = readArray(weights);
            for(int k = 0; k < layerSize[i-1]; k ++)
            {
                if (k < rec) allWeights[allWeightsAddr] = weights[k];
                else allWeights[allWeightsAddr] = 0;
                allWeightsAddr ++;
            }
            Serial0.print(rec);
            Serial0.println(" values received");
        }
    }
    int allBiasesSize = 0;
    for (int i = 1; i < layerNum; i ++){
        allBiasesSize += layerSize[i];
    }
    Serial0.print("Number of all Biases: ");
    Serial0.println(allBiasesSize);
    int allBiases[allBiasesSize];
    int allBiasesAddr = 0;
    for (int i = 1; i < layerNum; i ++)
    {
        Serial0.print("Biases for Layer ");
        Serial0.print(i+1);
        Serial0.print(" for neuron 1 to ");
        Serial0.print(layerSize[i]);
        Serial0.print(": ");
        int biases[layerSize[i]];
        int rec = readArray(biases);
        for(int k = 0; k < layerSize[i]; k ++)
        {
            if (k < rec) allBiases[allBiasesAddr] = biases[k];
            else allBiases[allBiasesAddr] = 0;
            allBiasesAddr ++;
        }
        Serial0.print(rec);
        Serial0.println(" values received");
    }
    
    Serial0.println("Erase Flash...");
    eraseFlash();
    Serial0.println("Flash Erased");
    
    Serial0.println("Save Weights...");
    int startAddr = 1000;
    long startMillis = millis();
    for (int i = 0; i < allWeightsAddr; i ++)
    {
        saveIntegerFlash(startAddr + i*4, allWeights[i]);
        if (millis()-startMillis > 10000)
        {
            Serial0.print(i);
            Serial0.print("/");
            Serial0.println(allWeightsAddr);
            startMillis = millis();
        }
    }
    Serial0.print("Saved ");
    Serial0.print(allWeightsAddr);
    Serial0.print(" weights from address ");
    Serial0.print(startAddr);
    Serial0.print(" to ");
    Serial0.println(startAddr + allWeightsAddr*4 - 1);
    
    Serial0.println("Save Biases...");
    startAddr += allWeightsAddr*4;
    startMillis = millis();
    for (int i = 0; i < allBiasesAddr; i ++)
    {
        saveIntegerFlash(startAddr + i*4, allBiases[i]);
        if (millis()-startMillis > 10000)
        {
            Serial0.print(i);
            Serial0.print("/");
            Serial0.println(allBiasesAddr);
            startMillis = millis();
        }
    }
    Serial0.print("Saved ");
    Serial0.print(allBiasesAddr);
    Serial0.print(" biases from address ");
    Serial0.print(startAddr);
    Serial0.print(" to ");
    Serial0.println(startAddr + allBiasesAddr*4 - 1);
}

int readInt(){
    int data = 0;
    bool negative = false;
    char c;
    while(c != '\n')
    {
        c = Serial0.read();
        if (c == '\n')
        {
            if (negative) data *= (-1);
        }
        else if (c > 47 && c < 58) {
            data *= 10;
            data += c-48;
        }
        else if (c == '-') {
            negative = true;
        }
    }
    return data;
}

int readArray(int* array){
    int addr = 0;
    int data = 0;
    bool negative = false;
    char c;
    while(c != '\n')
    {
        c = Serial0.read();
        if (c == ',' || c == '\n')
        {
            array[addr] = data;
            if (negative) array[addr] *= (-1);
            negative = false;
            data = 0;
            addr ++;
        }
        else if (c > 47 && c < 58) {
            data *= 10;
            data += c-48;
        }
        else if (c == '-') {
            negative = true;
        }
    }
    return addr;
}

void eraseFlash(){
    flash.chipErase();
    while(flash.busy());
}

void saveIntegerFlash(uint32_t addr, int data){
    byte d1 = data;
    byte d2 = data >> 8;
    byte d3 = data >> 16;
    byte d4 = data >> 24;

    flash.writeByte(addr, d1);
    flash.writeByte(addr+1, d2);
    flash.writeByte(addr+2, d3);
    flash.writeByte(addr+3, d4);
}

int readIntegerFlash(uint32_t addr){
    byte d1 = flash.readByte(addr);
    byte d2 = flash.readByte(addr+1);
    byte d3 = flash.readByte(addr+2);
    byte d4 = flash.readByte(addr+3);

    return d1 | d2 << 8 | d3 << 16  | d4 << 24;
}

