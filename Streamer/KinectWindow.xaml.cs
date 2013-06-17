//------------------------------------------------------------------------------
// <copyright file="KinectWindow.xaml.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Samples.Kinect.KinectExplorer
{
    using System;
    using System.Globalization;
    using System.IO;
    using System.Windows;
    using System.Windows.Media;
    using System.Windows.Media.Imaging;
    using System.Diagnostics;

    public partial class KinectWindow : Window
    {

        private WriteableBitmap colorBitmap;
        private String data;
        private KinectSensorItem kinect;
        private String name;

        public KinectWindow(WriteableBitmap _colorBitmap, String _name, String _data, KinectSensorItem _kinect)
        {
            InitializeComponent();
            this.colorBitmap = _colorBitmap;
            Title = _name;
            data = _data;
            name = _name;
            kinect = _kinect;
        }


        private void WindowLoaded(object sender, RoutedEventArgs e)
        {
            this.Image.Source = this.colorBitmap;
        }

        private void WindowClosing(object sender, System.ComponentModel.CancelEventArgs e)
        {

        }

        public void setFPS(int i)
        {
            Title = name + " (FPS: " + i.ToString() + ")";
        }
        private void EditSettings_Click(object sender, RoutedEventArgs e)
        {
            Process.Start("notepad.exe", data);
        }

        private void ReloadSettings_Click(object sender, RoutedEventArgs e)
        {
            kinect.LoadSettings();
        }        

    }
}
