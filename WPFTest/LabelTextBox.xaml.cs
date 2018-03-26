using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Windows;
using System.Windows.Controls;

namespace XZiar.WPFControl
{
    public sealed partial class LabelTextBox : UserControl
    {
        public static readonly DependencyProperty LabelProperty = DependencyProperty.Register("Label",
            typeof(string),
            typeof(LabelTextBox),
            new PropertyMetadata(default(string)));

        public static readonly DependencyProperty TextProperty = DependencyProperty.Register("Text",
            typeof(string),
            typeof(LabelTextBox),
            new PropertyMetadata(default(string)));

        public static readonly DependencyProperty IsReadOnlyProperty = DependencyProperty.Register("IsReadOnly",
            typeof(bool),
            typeof(LabelTextBox),
            new PropertyMetadata(false));

        public static readonly DependencyProperty LabelWidthProperty = DependencyProperty.Register("LabelWidth",
            typeof(GridLength),
            typeof(LabelTextBox),
            new PropertyMetadata(GridLength.Auto));

        public string Label
        {
            get { return (string)GetValue(LabelProperty); }
            set { SetValue(LabelProperty, value); }
        }

        public string Text
        {
            get { return (string)GetValue(TextProperty); }
            set { SetValue(TextProperty, value); }
        }

        public bool IsReadOnly
        {
            get { return (bool)GetValue(IsReadOnlyProperty); }
            set { SetValue(IsReadOnlyProperty, value); }
        }

        public GridLength LabelWidth
        {
            get { return (GridLength)GetValue(LabelWidthProperty); }
            set { SetValue(LabelWidthProperty, value); }
        }

        public LabelTextBox()
        {
            this.InitializeComponent();
        }
    }
}
