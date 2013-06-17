namespace Microsoft.Samples.Kinect.KinectExplorer
{
    using System;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Globalization;
    using System.Windows;
    using System.Windows.Documents;
    using System.Windows.Input;
    using Microsoft.Kinect;

    public partial class MainWindow : Window
    {
        private readonly KinectSensorItemCollection sensorItems;
        private readonly ObservableCollection<KinectStatusItem> statusItems;
        private int nrOfKinects = 0;

        public MainWindow()
        {
            InitializeComponent();
            this.sensorItems = new KinectSensorItemCollection();
            this.statusItems = new ObservableCollection<KinectStatusItem>();
            this.kinectSensors.ItemsSource = this.sensorItems;
            this.kinectStatus.ItemsSource = this.statusItems;
        }

        private void WindowLoaded(object sender, EventArgs e)
        {
            // listen to any status change for Kinects.
            KinectSensor.KinectSensors.StatusChanged += this.KinectsStatusChanged;

            // show status for each sensor that is found now.
            foreach (KinectSensor kinect in KinectSensor.KinectSensors)
            {
                this.ShowStatus(kinect, kinect.Status);
            }
        }

        private void WindowClosed(object sender, EventArgs e)
        {
            //kinects are only closed here!
            foreach (KinectSensorItem sensorItem in this.sensorItems)
            {
                sensorItem.Close();
            }

            this.sensorItems.Clear();
        }

        private void ShowStatus(KinectSensor kinectSensor, KinectStatus kinectStatus)
        {

            KinectSensorItem sensorItem;
            this.sensorItems.SensorLookup.TryGetValue(kinectSensor, out sensorItem);

            //kinect is gestopt
            if (KinectStatus.Disconnected == kinectStatus)
            {
                if (sensorItem != null)
                {
                    sensorItem.StreamingStatus = "Stopped";
                    this.sensorItems.Remove(sensorItem);
                    sensorItem.Close();
                }
            }
            else
            {
                //kinect is niew
                if (sensorItem == null)
                {
                    sensorItem = new KinectSensorItem(this, kinectSensor, kinectSensor.DeviceConnectionId, nrOfKinects);
                    sensorItem.Status = kinectStatus;

                    this.sensorItems.Add(sensorItem);
                }
                //kinect heeft een update
                else
                {
                    sensorItem.Status = kinectStatus;
                }
                //eens connected: show window
                if (KinectStatus.Connected == kinectStatus )
                {
                    // show a window by default: off
                    //sensorItem.ShowWindow();
                }
                //indien disconnected: close
                else
                {
                  //  sensorItem.Close();
                }
            }


            this.statusItems.Add(new KinectStatusItem
            {
                Id = (null == kinectSensor) ? null : kinectSensor.DeviceConnectionId,
                Status = kinectStatus,
                DateTime = DateTime.Now,
                Name = (sensorItem != null) ? sensorItem.Name : "Loading...",
                StreamingStatus = (sensorItem != null) ? sensorItem.StreamingStatus : "Not Ready"

            });


        }

        public void updateStreamingStats(KinectSensor kinectSensor, KinectStatus kinectStatus)
        {
            KinectSensorItem sensorItem;
            this.sensorItems.SensorLookup.TryGetValue(kinectSensor, out sensorItem);
            if (null != kinectSensor)
            {
                this.statusItems.Add(new KinectStatusItem
                {
                    Id = (null == kinectSensor) ? null : kinectSensor.DeviceConnectionId,
                    Status = kinectStatus,
                    DateTime = DateTime.Now,
                    Name = (sensorItem != null) ? sensorItem.Name : "Loading...",
                    StreamingStatus = (sensorItem != null) ? sensorItem.StreamingStatus : "Not Ready"

                });
            }

            this.sensorItems.Remove(sensorItem);
            this.sensorItems.Add(sensorItem);
        }

        private void KinectsStatusChanged(object sender, StatusChangedEventArgs e)
        {
            this.ShowStatus(e.Sensor, e.Status);
        }


        private void Sensor_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            var element = sender as FrameworkElement;

            if (null == element)
            {
                return;
            }

            var sensorItem = element.DataContext as KinectSensorItem;

            if (null == sensorItem)
            {
                return;
            }

            sensorItem.ShowWindow();
        }
    }
}
