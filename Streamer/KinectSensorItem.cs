//------------------------------------------------------------------------------
// <copyright file="KinectSensorItem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Samples.Kinect.KinectExplorer
{
    using System;
    using System.ComponentModel;
    using Microsoft.Kinect;
    using System.Xml;
    using System.Text;
    using System.Globalization;
    using System.IO;
    using System.Windows;
    using System.Windows.Media;
    using System.Windows.Media.Imaging;
    using System.Threading;
    using System.Net;
    using System.Collections;
    using System.Net.Sockets;
    using System.Diagnostics;
    using System.Threading.Tasks;
    using System.IO.MemoryMappedFiles;
    using System.Runtime.InteropServices;

    public class KinectSensorItem : INotifyPropertyChanged
    {
        /** Memory mapped files **/
        MemoryMappedFile mmf;

        /** SETTINGS **/
        int skeletonSmoothing = 0;

        /** KINECT **/
        public KinectSensor Sensor { get; private set; }
        public string Id { get; private set; }
        public string USBID { get; private set; }
        public string Name { get; private set; }
        public string StreamingStatus { get; set; }
        public int kinectNr { get; set; }  
        DateTime lastTime;


        private WriteableBitmap depthBitmap;
        private DepthImagePixel[] depthPixels;
        private ColorImagePoint[] colorCoordinates;
        private DepthImagePoint[] depthPoints;
        private byte[] depthPreview;
        private byte[] colorPixelsAligned;

        private short[] pixelsToSendD;
        private byte[] pixelsToSendDByte;
        private byte[] pixelsToSendRGB;
        private byte[] emptyRGB;

        private short[] pixelsToSendWorld;
        private byte[] pixelsToSendWorldByte;
        private SkeletonPoint[] worldPoints;
        
        private byte[] intensityLookupTable;

        /** CALLBACKS **/
        MainWindow controller;

        public KinectSensorItem(MainWindow _controller, KinectSensor Sensor, string id, int kinectN)
        {
            this.controller = _controller;
            this.Sensor = Sensor;
            this.Id = id;
            this.USBID = RemoveSpecialCharacters(this.Id);
            this.kinectNr = kinectN;
            Name = "Default Kinect";
            StreamingStatus = "Not Ready";
            LoadSettings();

            int tries = 0;
            if (null != this.Sensor)
            {
                while (this.Sensor.Status != KinectStatus.Connected)
                {
                    System.Threading.Thread.Sleep(1000);
                    tries++;
                    if (tries == 4)
                        break;

                }
            }
            if (null != this.Sensor && this.Sensor.Status == KinectStatus.Connected)
            {
                //streams
                this.Sensor.DepthStream.Enable(DepthImageFormat.Resolution640x480Fps30);
                this.Sensor.ColorStream.Enable(ColorImageFormat.RgbResolution640x480Fps30);     
           
                //the preview window
                this.depthPreview           = new byte[this.Sensor.DepthStream.FramePixelDataLength];
                
                //original input
                this.depthPixels            = new DepthImagePixel[this.Sensor.DepthStream.FramePixelDataLength];
                this.colorPixelsAligned     = new byte[this.Sensor.ColorStream.FramePixelDataLength];

                //alignment Data
                this.colorCoordinates       = new ColorImagePoint[this.Sensor.DepthStream.FramePixelDataLength];
                this.depthPoints            = new DepthImagePoint[this.Sensor.DepthStream.FramePixelDataLength];

                //World & depth data (shorts)
                this.pixelsToSendWorld      = new short[this.Sensor.DepthStream.FramePixelDataLength * 3]; //x
                this.pixelsToSendD          = new short[this.Sensor.DepthStream.FramePixelDataLength];
                this.worldPoints            = new SkeletonPoint[this.Sensor.DepthStream.FramePixelDataLength];

                //thnings that actually will be send
                this.pixelsToSendRGB        = new byte[this.Sensor.DepthStream.FramePixelDataLength * 3];
                this.pixelsToSendDByte      = new byte[pixelsToSendD.Length * sizeof(short)];            
                this.pixelsToSendWorldByte  = new byte[pixelsToSendWorld.Length * sizeof(short)];
                this.emptyRGB = new byte[this.Sensor.DepthStream.FramePixelDataLength * 3];

                //some kinect stuff
                this.depthBitmap = new WriteableBitmap(this.Sensor.DepthStream.FrameWidth, this.Sensor.DepthStream.FrameHeight, 96.0, 96.0, PixelFormats.Gray8, null);
                this.Sensor.SkeletonStream.Enable();
                this.Sensor.AllFramesReady += SensorAllFramesReady;

                //the memory map
                this.mmf = MemoryMappedFile.CreateNew("kinect" + kinectN, 100 + this.Sensor.DepthStream.FramePixelDataLength * 11); //3 rgb, 2 depth, 3x2 world + 100 txt

                //pre-allocate the empty array
                for (int i = 0; i < emptyRGB.Length; i++) 
                    emptyRGB[i] = 0;

                try
                {
                    this.Sensor.Start();
                }
                catch (IOException)
                {
                    this.Sensor = null;
                }
            }

            if (null == this.Sensor)
            {
                setStreamingStatus("Unable to connect");
            }
            else
            {
                setStreamingStatus("MMF = [kinect" + kinectNr + "]");
            }

            intensityLookupTable = new byte[8193];
            for (int i = 0; i < 8193; i++)
            {
                intensityLookupTable[i] = (byte)(((255 - (255 * i / 8192))));
            }

            lastTime = DateTime.Now;

            SaveSettings();
        }

        long frameNr = 0;

        private void SensorAllFramesReady(object sender, AllFramesReadyEventArgs e)
        {

            if (null == this.Sensor)
            {
                return;
            }

            bool gotColor = false;
            bool gotDepth = false;

            Stopwatch stopwatch = new Stopwatch();

            stopwatch.Start();
            using (ColorImageFrame colorFrame = e.OpenColorImageFrame())
            {
                if (colorFrame != null)
                {
                    colorFrame.CopyPixelDataTo(this.colorPixelsAligned);
                    gotColor = true;
                }
            }

            using (DepthImageFrame depthFrame = e.OpenDepthImageFrame())
            {
                if (depthFrame != null)
                {
                    depthFrame.CopyDepthImagePixelDataTo(this.depthPixels);
                    depthFrame.CopyPixelDataTo(pixelsToSendD);
                    gotDepth = true;
                }
            }


            if (gotColor && gotDepth)
            {                
                //map before smoothing
                this.Sensor.CoordinateMapper.MapDepthFrameToColorFrame(DepthImageFormat.Resolution640x480Fps30, depthPixels, ColorImageFormat.RgbResolution640x480Fps30, colorCoordinates);
                this.Sensor.CoordinateMapper.MapDepthFrameToSkeletonFrame(DepthImageFormat.Resolution640x480Fps30, depthPixels, worldPoints);
                
                //first: aligned RGB-D frame
                System.Buffer.BlockCopy(emptyRGB, 0, pixelsToSendRGB, 0, emptyRGB.Length);
 
                //rest
                int maxLength = colorPixelsAligned.Length - 5;
                int totalLength = this.depthPixels.Length;
                int maxThreads = 4; //4 seems to be the best number here
                int blockLength = totalLength / maxThreads;
                Parallel.For(0, maxThreads, t =>
                {
                    int blockstart = t * blockLength;
                    int blockend = (t + 1) * blockLength;
                    for (int i = blockstart; i < blockend; ++i)
                    {
                        //align
                        int baseIndex = (colorCoordinates[i].Y * 640 + colorCoordinates[i].X) * 4;
                        if (baseIndex < maxLength) //check takes like 5 ms per frame
                        {
                            System.Buffer.BlockCopy(colorPixelsAligned, baseIndex, pixelsToSendRGB, i*3, 3); //takes 7 ms per frame...
                        }

                        //depth image
                        depthPreview[i] = (byte)(pixelsToSendD[i] >> 8);
                        
                        //world image
                        if (KinectSensor.IsKnownPoint(worldPoints[i]))
                        {
                            baseIndex = i * 3;
                            pixelsToSendWorld[baseIndex++] = (short)(1000 * worldPoints[i].X);
                            pixelsToSendWorld[baseIndex++] = (short)(1000 * worldPoints[i].Y);
                            pixelsToSendWorld[baseIndex++] = (short)(pixelsToSendD[i] >> 3);

                        }                         
                    }
                });

                //copy into mem
                Buffer.BlockCopy(pixelsToSendD, 0, pixelsToSendDByte, 0, pixelsToSendDByte.Length);
                Buffer.BlockCopy(pixelsToSendWorld, 0, pixelsToSendWorldByte, 0, pixelsToSendWorldByte.Length);

                //shared memory
                bool mutexCreated;
                Mutex mutex = new Mutex(true, "testmapmutex", out mutexCreated);
                String str = Name + " FN" + frameNr++;
                byte[] namebytes = new byte[100];
                for (int i = 0; i < 100; i++) namebytes[i] = 0;
                System.Buffer.BlockCopy(str.ToCharArray(), 0, namebytes, 0, str.Length * sizeof(char));
                using (MemoryMappedViewStream stream = mmf.CreateViewStream())
                {
                    BinaryWriter writer = new BinaryWriter(stream);
                    writer.Write(namebytes);
                    writer.Write(pixelsToSendRGB);
                    writer.Write(pixelsToSendDByte);
                    writer.Write(pixelsToSendWorldByte);
                }
                mutex.WaitOne();
                mutex.ReleaseMutex();
                
                CalculateFps();

                // Write the pixel data into our bitmap
                this.depthBitmap.WritePixels(
                    new Int32Rect(0, 0, this.depthBitmap.PixelWidth, this.depthBitmap.PixelHeight),
                    this.depthPreview,
                    this.depthBitmap.PixelWidth * sizeof(byte),
                    0);

                Console.WriteLine("KinectFrameProcessing: " + stopwatch.ElapsedMilliseconds);
            }
        }

        public void setStreamingStatus(String s)
        {
            try
            {
                App.Current.Dispatcher.Invoke((Action)(() =>
                {
                    StreamingStatus = s;
                    controller.updateStreamingStats(Sensor, Sensor.Status);
                }));
            }
            catch (Exception e)
            {
            }
        }





        /** KINECT STATUS **/
        /// <summary>
        /// The last set status.
        /// </summary>
        private KinectStatus status;
        public event PropertyChangedEventHandler PropertyChanged;
        public KinectStatus Status
        {
            get
            {
                return this.status;
            }
            set
            {
                if (this.status != value)
                {
                    this.status = value;
                    this.NotifyPropertyChanged("Status");
                }
            }
        }
        private void NotifyPropertyChanged(string propertyName)
        {
            if (this.PropertyChanged != null)
            {
                this.PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }


        /** WINDOW MANAGING **/
        private KinectWindow Window;

        public void ShowWindow()
        {
            if (null == this.Window)
            {
                var kinectWindow = new KinectWindow(depthBitmap, Name, "data/kinect/" + USBID + ".xml", this);
                kinectWindow.Closed += this.KinectWindowOnClosed;
                this.Window = kinectWindow;
            }

            //this.Window.KinectSensor = this.Sensor;
            this.Window.Show();
            this.Window.Activate();
        }


        public void Close()
        {
            //dismisses the window
            if (null != this.Window)
            {
                this.Window.Close();
                this.Window = null;
            }

            //closes the Sensor
            if ((null != this.Sensor) && this.Sensor.IsRunning)
            {
                this.Sensor.Stop();
            }
            this.Sensor = null;

            mmf.Dispose();

            // saves the settings
            SaveSettings();
        }

        //user clicked the window away
        private void KinectWindowOnClosed(object sender, EventArgs e)
        {
            //this does NOT close the sensor but just dismisses the window reference.
            this.Window.Closed -= this.KinectWindowOnClosed;
            this.Window = null;
        }




        /**** SETTINGS ****/

        /// <summary>
        /// Loads the XML settings from file
        /// </summary>
        /// <returns>True if succesful, false if unsuccesfull (file not found).</returns>
        public bool LoadSettings()
        {
            // settings
            XmlDocument xml = new XmlDocument();
            try
            {
                xml.Load("data/kinect/" + USBID + ".xml");
                XmlNodeList stngs = xml.SelectNodes("//Settings");
                foreach (XmlNode stng in stngs)
                {
                    Name = stng.SelectSingleNode("Name").InnerText;
                    /*
                    depthDataServerPort = Int16.Parse(stng.SelectSingleNode("depthDataServerPort").InnerText);
                    sendSkeletonDataPort = Int16.Parse(stng.SelectSingleNode("sendSkeletonDataPort").InnerText);
                    controlKinectIncomingPort = Int16.Parse(stng.SelectSingleNode("controlKinectIncomingPort").InnerText);
                    controlKinectOutgoingPort = Int16.Parse(stng.SelectSingleNode("controlKinectOutgoingPort").InnerText);
                    depthSmoothing = Int16.Parse(stng.SelectSingleNode("depthSmoothing").InnerText);
                    depthAveregeing = Int16.Parse(stng.SelectSingleNode("depthAveregeing").InnerText);
                     * */
                    skeletonSmoothing = Int16.Parse(stng.SelectSingleNode("skeletonSmoothing").InnerText);
                }
                setStreamingStatus("MMF = [kinect" + kinectNr + "]");
            }
            catch (Exception e)
            {
                skeletonSmoothing = 0;
                return false;
            }

            return true;
        }

        /// <summary>
        /// Saves the settings to the xml file
        /// </summary>
        public void SaveSettings()
        {
            XmlWriterSettings settings = new XmlWriterSettings();
            settings.Indent = true;

            Directory.CreateDirectory("data/kinect/");
            XmlWriter writer = XmlWriter.Create("data/kinect/" + USBID + ".xml", settings);
            writer.WriteStartDocument();
            writer.WriteComment("This file is generated automatically after inserting a kinect in a new USB port and reverts to its defaults.");
            writer.WriteComment("---- KINECT LEFT OR RIGHT? ----");
            writer.WriteStartElement("Settings");
            writer.WriteElementString("Name", Name);
            writer.WriteComment("0 = disable; 1 = low smoothing (& delay), 9 = max smoothing");
            writer.WriteElementString("skeletonSmoothing", skeletonSmoothing + "");
            writer.WriteEndElement();
            writer.WriteEndDocument();
            writer.Flush();
            writer.Close();
        }


        /****** HULPMETHODES **********/
        public string RemoveSpecialCharacters(string str)
        {
            StringBuilder sb = new StringBuilder();
            foreach (char c in str)
            {
                if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '.' || c == '_')
                {
                    sb.Append(c);
                }
            }
            return sb.ToString();
        }

        int totalFrames = 0;
        int lastFrames = 0;

        int FPS;

        void CalculateFps()
        {
            ++totalFrames;

            var cur = DateTime.Now;
            if (cur.Subtract(lastTime) > TimeSpan.FromSeconds(1))
            {
                int frameDiff = totalFrames - lastFrames;
                lastFrames = totalFrames;
                lastTime = cur;
                FPS = frameDiff;

                if (null != this.Window)
                {
                    this.Window.setFPS(FPS);
                }
            }
        }  
    }
}
