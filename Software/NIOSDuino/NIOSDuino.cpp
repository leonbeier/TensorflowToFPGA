
#include <Arduino.h>
#include <SPIFlash.h>
#include <altera_avalon_pio_regs.h>

SPIFlash flash(31);

#define flashStartAddress 100

void eraseFlash();
void saveIntegerFlash(uint32_t addr, int data);
int readIntegerFlash(uint32_t addr);
int readArray(int* array);
void configNN();
void loadNN();
int readInt();
uint16_t neuronAddress();
bool loadedInputData();
uint16_t inputAddress();
bool neuronDataValid();
void setAddress(uint32_t address);
void startCalculation(uint32_t inputValues);
void nextNeuron(uint32_t neuronAddress);
void nextLayer(uint32_t newInputNumber);
void finishCalculation();
void setWeight(int value);
void setBias(int value);
void runNN();

int layerNum = 0;
int* layerSize;
int* allWeights;
int* allBiases;

void setup() {
    Serial0.begin(115200);
    
    if (flash.initialize()) Serial0.println("Init OK!");
    else Serial0.println("Init FAIL!");
}

void loop() {
    Serial0.println("l = Load neural net from flash\nc = Configure new neural net\ns = Start neural net");
    char c = Serial0.read();
    while(Serial0.read() != '\n');
    switch (c) {
        case 'l':
            loadNN();
            break;
        case 'c':
            configNN();
            break;
        case 's':
            runNN();
            break;
    }
}

bool validValue  = false;
bool validValueN = true;

void runNN(){
    validValue = digitalRead(50);
    validValueN = !validValue;
    
    Serial0.println("Run neural net");
    while (!loadedInputData());
    Serial0.println("Loaded input data");
    if(layerNum > 0){
        startCalculation(layerSize[0]);
        Serial0.println("Started calculation");
        int allWeightsAddr = 0;
        int allBiasesAddr = 0;
        for (int layer = 1; layer < layerNum; layer++) {
            Serial0.print("Layer ");
            Serial0.println(layer);
            for (int neuron = 0; neuron < layerSize[layer]; neuron++) {
                Serial0.print("Neuron ");
                Serial0.print(neuron+1);
                Serial0.print("/");
                Serial0.println(layerSize[layer]);
                if(neuron > 0) nextNeuron(neuron);
                int maxAddr = allWeightsAddr + layerSize[layer-1] - 1;
                setBias(allBiases[allBiasesAddr++]);
                while (allWeightsAddr < maxAddr) {
                    setWeight(allWeights[allWeightsAddr++]);
                    digitalWrite(50, validValueN);
                    setWeight(allWeights[allWeightsAddr++]);
                    digitalWrite(50, validValue);
                }
                if (allWeightsAddr < maxAddr+1) {
                    setWeight(allWeights[allWeightsAddr++]);
                    digitalWrite(50, validValueN);
                    validValue  = !validValue;
                    validValueN = !validValueN;
                }
            }
            nextNeuron(0);
            Serial0.println("reset neuron");
            nextLayer(layerSize[layer]);
            Serial0.println("copy layer");
        }
        finishCalculation();
    }
    Serial0.println("Finished");
}

//First reads input data for neural net in the internal RAM. This returns true if the data is fully loaded
bool loadedInputData(){
    return digitalRead(49);
}

//To calculate one value of a neuron, has to go through all inputs. This returns the current input
uint16_t inputAddress(){
    return IORD_ALTERA_AVALON_PIO_DATA(PIO_1_BASE) & 0xFFFF;
}

//Retruns true if value of neuron is calculated and the weights for the next neuron can be sent
bool neuronDataValid(){
    return digitalRead(48);
}

void setAddress(uint32_t address){
    uint32_t i = IORD_ALTERA_AVALON_PIO_DATA(PIO_2_BASE) & 0xFFFE0000;
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_2_BASE,  i + address);
}

//Starts calculation of neural net and sets number of input values for neural net
void startCalculation(uint32_t inputValues){
    setAddress(inputValues);
    digitalWrite(49, HIGH);
    digitalWrite(49, LOW);
}

//After sending the weights for the last neuron, sets address of next neuron and waits until value of last neuron is calculated
void nextNeuron(uint32_t neuronAddress){
    setAddress(neuronAddress);
    digitalWrite(51, HIGH);
    digitalWrite(51, LOW);
    while (!neuronDataValid());
}

//After one layer of neurons is calculated, sets new number of inputs (number of calculated neurons from last layer) and waits until the data is moved from the output to the input RAM
void nextLayer(uint32_t newInputNumber){
    setAddress(newInputNumber);
    digitalWrite(52, HIGH);
    digitalWrite(52, LOW);
    while (!neuronDataValid());
}

//Indicates that calculation is finished and the output can now be read
void finishCalculation(){
    digitalWrite(53, HIGH);
    digitalWrite(53, LOW);
}

void setWeight(int value){
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_0_BASE, value);
    
}

void setBias(int value){
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_3_BASE, value);
}

void loadNN(){
    Serial0.println("Load Neural Net from Flash...");
    Serial0.print("Number of layers: ");
    int startAddr = flashStartAddress;
    Serial0.println(layerNum = readIntegerFlash(startAddr));
    startAddr += 4;
    layerSize = new int[layerNum];
    for (int i = 0; i < layerNum; i ++)
    {
        Serial0.print("Neurons in layer ");
        Serial0.print(i+1);
        Serial0.print(": ");
        Serial0.println(layerSize[i] = readIntegerFlash(startAddr+i*4));
    }
    int allWeightsSize = 0;
    for (int i = 1; i < layerNum; i ++){
        allWeightsSize += layerSize[i-1] * layerSize[i];
    }
    Serial0.print("Number of all weights: ");
    Serial0.println(allWeightsSize);
    allWeights = new int[allWeightsSize];
    int allWeightsAddr = 0;
    startAddr += layerNum*4;
    for (int i = 1; i < layerNum; i ++) {
        for (int j = 0; j < layerSize[i]; j++) {
            
            Serial0.print("Layer ");
            Serial0.print(i+1);
            Serial0.print(" neuron ");
            Serial0.print(j+1);
            Serial0.print(" weights: ");
            
            for(int k = 0; k < layerSize[i-1]; k ++) {
                allWeights[allWeightsAddr] = readIntegerFlash(startAddr+allWeightsAddr*4);
                
                Serial0.print(allWeights[allWeightsAddr]);
                if (k < layerSize[i-1]-1) Serial0.print(", ");
                else Serial0.print("\n");
                
                allWeightsAddr ++;
            }
        }
    }
    int allBiasesSize = 0;
    for (int i = 1; i < layerNum; i ++){
        allBiasesSize += layerSize[i];
    }
    Serial0.print("Number of all Biases: ");
    Serial0.println(allBiasesSize);
    allBiases = new int[allBiasesSize];
    int allBiasesAddr = 0;
    startAddr += allWeightsAddr*4;
    for (int i = 1; i < layerNum; i ++)
    {
        /*
        Serial0.print("Layer ");
        Serial0.print(i+1);
        Serial0.print(" biases: ");
         */
        for(int k = 0; k < layerSize[i]; k ++)
        {
            allBiases[allBiasesAddr] = readIntegerFlash(startAddr+allBiasesAddr*4);
            /*
            Serial0.print(allBiases[allBiasesAddr]);
            if (k < layerSize[i]-1) Serial0.print(", ");
            else Serial0.print("\n");
             */
            allBiasesAddr ++;
        }
    }
}

void configNN (){
    Serial0.print("Number of total layers: ");
    Serial0.println(layerNum = readInt());
    layerSize = new int[layerNum];
    for (int i = 0; i < layerNum; i ++)
    {
        Serial0.print("Neurons in layer ");
        Serial0.print(i+1);
        Serial0.print(": ");
        Serial0.println(layerSize[i] = readInt());
    }
    int allWeightsSize = 0;
    for (int i = 1; i < layerNum; i ++){
        allWeightsSize += layerSize[i-1] * layerSize[i];
    }
    Serial0.print("Number of all weights: ");
    Serial0.println(allWeightsSize);
    allWeights = new int[allWeightsSize];
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
    allBiases = new int[allBiasesSize];
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
    
    //Save Layer Number
    Serial0.println("Save Number of Layers...");
    int startAddr = flashStartAddress;
    saveIntegerFlash(startAddr, layerNum);
    
    //Save Layer Sizes
    Serial0.println("Save Layer Sizes...");
    startAddr += 4;
    for (int i = 0; i < layerNum; i++) {
        saveIntegerFlash(startAddr + i*4, layerSize[i]);
    }
    
    //Save Weights
    Serial0.println("Save Weights...");
    startAddr += layerNum*4;
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
    
    //Save Biases
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

