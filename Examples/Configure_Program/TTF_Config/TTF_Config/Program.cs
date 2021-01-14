using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO.Ports;
using System.Threading;
using System.IO;
using System.Globalization;

namespace TTF_Config
{
    public class PortChat
    {
        static bool _continue;
        static SerialPort _serialPort;

        public static void Main()
        {
            List<int> layerSizes = new List<int>();
            List<string> weightFiles = new List<string>();
            List<string> biasFiles = new List<string>();

            Console.WriteLine("Number of layers: ");
            Int32.TryParse(Console.ReadLine(), out int layerNum);
            for(int i = 0; i < layerNum; i++)
            {
                Console.WriteLine("Number of neurons in layer " + (i+1) + ": ");
                Int32.TryParse(Console.ReadLine(), out int size);
                layerSizes.Add(size);
            }

            for (int i = 1; i < layerNum; i++)
            {
                Console.WriteLine("File path for weights for layer " + (i+1) + ": ");
                weightFiles.Add(Console.ReadLine());
            }

            for (int i = 1; i < layerNum; i++)
            {
                Console.WriteLine("File path for biases for layer " + (i + 1) + ": ");
                biasFiles.Add(Console.ReadLine());
            }

            // Create a new SerialPort object with default settings.
            _serialPort = new SerialPort();

            // Allow the user to set the appropriate properties.
            _serialPort.PortName = SetPortName(_serialPort.PortName);
            _serialPort.BaudRate = 38400;
            _serialPort.Parity = Parity.None;
            _serialPort.DataBits = 8;
            _serialPort.StopBits = StopBits.One;
            _serialPort.Handshake = Handshake.None;

            // Set the read/write timeouts
            _serialPort.ReadTimeout = 1000;
            _serialPort.WriteTimeout = 5000;

            _serialPort.Open();

            try
            {
                while (true) Console.WriteLine("\"" + _serialPort.ReadLine() + "\"");
            }
            catch (Exception){}

            received = false;

            while (!received)
            {
                _serialPort.WriteLine("c");
                received = WaitForResponse("Number of total layers", true);
            }

            _serialPort.WriteLine(layerNum+"");
            WaitForResponse("Neurons in layer", true);

            for (int i = 0; i < layerNum; i++)
            {
                _serialPort.WriteLine(layerSizes[i]+"");
                if (i < layerNum - 1) WaitForResponse("Neurons in layer", true);
                else WaitForResponse("Weights for Neuron", true);
            }

            int fi = 0;
            foreach (var f in weightFiles)
            {
                string text = "";
                try
                {
                    text = File.ReadAllText(f);
                }catch(Exception)
                {
                    Console.WriteLine("File " + f + " is in use. Please close the file in a different program and hit enter");
                    try
                    {
                        text = File.ReadAllText(f);
                    }catch(Exception)
                    {
                        Console.WriteLine("File still in use. File ignored");
                    }
                }
                string[] weightsInput = text.Split('\n'); //784 inputs -> 128 Outputs: 1. value = weights for 1. input and 128 ouputs
                List<List<int>> weights = new List<List<int>>();
                foreach(var i in weightsInput)
                {
                    string[] weightsOutput = i.Split(',');
                    int co = 0;
                    foreach(var w in weightsOutput)
                    {
                        double wf = 0;
                        Double.TryParse(w, NumberStyles.Float, CultureInfo.CreateSpecificCulture("en-US"), out wf);
                        wf *= 32767;
                        if (weights.Count < weightsOutput.Length) weights.Add(new List<int>());
                        weights[co].Add((int)Math.Round(wf, 0));
                        co++;
                    }
                }
                int wi = 0;
                Console.WriteLine("Send " + weights.Count() + " messages");
                foreach (var w in weights)
                {
                    received = false;
                    int maxLength = layerSizes[fi];
                    int num = 0;
                    byte[] buffer = new byte[maxLength*2+2];
                    buffer[num++] = 0x80; //32768 on start of every data set (values are from -32767 to 32767)
                    buffer[num++] = 0x00;
                    foreach (var wv in w)
                    {
                        if (num < maxLength * 2 + 1)
                        {
                            buffer[num++] = (byte)((wv >> 8) & 0xff);
                            buffer[num++] = (byte)(wv & 0xff);
                        }
                    }

                    Console.WriteLine((wi+1) + ": Send " + num/2 + " values");

                    bool error;
                    do
                    {
                        _serialPort.Write(buffer, 0, num);

                        if (wi < weights.Count() - 1 || fi < weightFiles.Count() - 1) error = !WaitForResponse("Weights for Neuron", true, true);
                        else error = !WaitForResponse("Biases for Layer", true, true);

                        if (error) Console.WriteLine("Error-------------------------------------------------------------------------");
                    } while (error);
                    
                    //Send 0x0000 until receives (Should not end up here)
                    if (error)
                    {
                        Console.WriteLine("Send 0x0000");
                        byte[] b = { 0x80, 0x00 };
                        _serialPort.Write(b, 0, 2);
                        while(WaitForResponse());
                    }

                    wi++;
                }
                fi++;
            }

            fi = 0;
            Console.WriteLine("Send " + biasFiles.Count() + " messages");
            foreach (var f in biasFiles)
            {
                string text = "";
                try
                {
                    text = File.ReadAllText(f);
                }
                catch (Exception)
                {
                    Console.WriteLine("File " + f + " is in use. Please close the file in a different program and hit enter");
                    try
                    {
                        text = File.ReadAllText(f);
                    }
                    catch (Exception)
                    {
                        Console.WriteLine("File still in use. File ignored");
                    }
                }
                string[] biasesText = text.Split('\n');

                List<int> biases = new List<int>();
                foreach (var i in biasesText)
                {
                    double wf = 0;
                    Double.TryParse(i, NumberStyles.Float, CultureInfo.CreateSpecificCulture("en-US"), out wf);
                    wf *= 32767;
                    biases.Add((int)Math.Round(wf, 0));
                }

                received = false;
                int maxLength = layerSizes[fi+1];
                int num = 0;
                byte[] buffer = new byte[maxLength * 2 + 2];
                buffer[num++] = 0x80; //32768 on start of every data set (values are from -32767 to 32767)
                buffer[num++] = 0x00;
                foreach (var wv in biases)
                {
                    if (num < maxLength * 2 + 1)
                    {
                        buffer[num++] = (byte)((wv >> 8) & 0xff);
                        buffer[num++] = (byte)(wv & 0xff);
                    }
                }

                Console.WriteLine((fi+1) + ": Send " + num / 2 + " values");

                bool error;
                do
                {
                    _serialPort.Write(buffer, 0, num);

                    if (fi < biasFiles.Count() - 1) error = !WaitForResponse("Biases for Layer", true, true);
                    else error = !WaitForResponse("Erase Flash...", true, true);

                    if (error) Console.WriteLine("Error-------------------------------------------------------------------------");
                } while (error);
                fi++;
            }

            while (true)
            {
                try
                {
                    string message = _serialPort.ReadLine();
                    receiveMessage = message;
                    Console.WriteLine(message);
                    received = true;
                }
                catch (TimeoutException) { }
            }
        }

        //Auswahl -> return false bei anderer Response "firstResponse"
        public static bool WaitForResponse(string response = "", bool printResponse = false, bool firstResponse = false)
        {
            try
            {
                while (true)
                {
                    receiveMessage = _serialPort.ReadLine();
                    if (printResponse) Console.WriteLine(receiveMessage);
                    if (response == "" || receiveMessage.Contains(response)) return true;
                    else if (firstResponse) return false;
                }
            }
            catch (Exception) {
                return false;
            }
            
        }

        public static bool received { get; set; } = false;

        public static string receiveMessage { get; set; } = "";

        // Display Port values and prompt user to enter a port.
        public static string SetPortName(string defaultPortName)
        {
            string portName;

            Console.WriteLine("Available Ports:");
            foreach (string s in SerialPort.GetPortNames())
            {
                Console.WriteLine("   {0}", s);
            }

            Console.Write("Enter COM port value (Default: {0}): ", defaultPortName);
            portName = Console.ReadLine();

            if (portName == "" || !(portName.ToLower()).StartsWith("com"))
            {
                portName = defaultPortName;
            }
            return portName;
        }

    }
}
