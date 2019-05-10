using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;


namespace XZiar.WPFControl
{
    public class LabelControl : ContentControl
    {
        internal static readonly ResourceDictionary ResDict;
        private static readonly ControlTemplate LabelControlTemplate;

        public static readonly DependencyProperty LabelProperty = DependencyProperty.Register("Label",
            typeof(string),
            typeof(LabelControl),
            new PropertyMetadata(default(string)));

        public static readonly DependencyProperty LabelWidthProperty = DependencyProperty.Register("LabelWidth",
            typeof(GridLength),
            typeof(LabelControl),
            new PropertyMetadata(GridLength.Auto));

        public static readonly DependencyProperty LabelFontSizeProperty = DependencyProperty.Register("LabelFontSize",
            typeof(double),
            typeof(LabelControl),
            new PropertyMetadata(FontSizeProperty.DefaultMetadata.DefaultValue));

        public string Label
        {
            get { return (string)GetValue(LabelProperty); }
            set { SetValue(LabelProperty, value); }
        }

        public GridLength LabelWidth
        {
            get { return (GridLength)GetValue(LabelWidthProperty); }
            set { SetValue(LabelWidthProperty, value); }
        }

        public double LabelFontSize
        {
            get { return (double)GetValue(LabelFontSizeProperty); }
            set { SetValue(LabelFontSizeProperty, value); }
        }

        static LabelControl()
        {
            //DefaultStyleKeyProperty.OverrideMetadata(typeof(LabelControl), new FrameworkPropertyMetadata(typeof(LabelControl)));
            ResDict = new ResourceDictionary { Source = new Uri("WPFControlExt;component/LabelControl.res.xaml", UriKind.RelativeOrAbsolute) };
            LabelControlTemplate = (ControlTemplate)ResDict["LabelControlTemplate"];
            TemplateProperty.OverrideMetadata(typeof(LabelControl), new FrameworkPropertyMetadata(LabelControlTemplate));
        }

        public LabelControl()
        {
            Template = LabelControlTemplate;
            ApplyTemplate();
        }
    }

    public class LabelTextBox : LabelControl
    {
        private static readonly DataTemplate ReadonlyLabelTextTemplate;
        private static readonly DataTemplate EditableLabelTextTemplate;

        public static readonly DependencyProperty IsReadOnlyProperty = DependencyProperty.Register("IsReadOnly",
            typeof(bool),
            typeof(LabelTextBox),
            new PropertyMetadata(false, OnIsReadOnlyChanged));
        private static void OnIsReadOnlyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            ((LabelTextBox)d).ContentTemplate = (bool)e.NewValue ? ReadonlyLabelTextTemplate : EditableLabelTextTemplate;
        }

        public static readonly DependencyProperty ContentToolTipProperty = DependencyProperty.Register("ContentToolTip",
            typeof(string),
            typeof(LabelTextBox),
            new PropertyMetadata(default(string)));

        public bool IsReadOnly
        {
            get { return (bool)GetValue(IsReadOnlyProperty); }
            set { SetValue(IsReadOnlyProperty, value); }
        }

        public string ContentToolTip
        {
            get { return (string)GetValue(ContentToolTipProperty); }
            set { SetValue(ContentToolTipProperty, value); }
        }

        static LabelTextBox()
        {
            DefaultStyleKeyProperty.OverrideMetadata(typeof(LabelTextBox), new FrameworkPropertyMetadata(typeof(LabelTextBox)));
            ReadonlyLabelTextTemplate = (DataTemplate)ResDict["ReadonlyLabelTextTemplate"];
            EditableLabelTextTemplate = (DataTemplate)ResDict["EditableLabelTextTemplate"];
        }
        public LabelTextBox()
        {
            ContentTemplate = ReadonlyLabelTextTemplate;
        }
    }

}