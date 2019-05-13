using Common;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
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
using XZiar.WPFControl;

namespace WPFTest
{
    [ContentProperty(nameof(Children))]
    public class SelectionControlGrid : ContentControl
    {
        internal static readonly ResourceDictionary ResDict;
        internal static readonly ControlTemplate SelCtrlGridTemplate;
        private static readonly PropertyPath DataContextPath = new PropertyPath(DataContextProperty);
        public static readonly DependencyProperty TargetProperty = DependencyProperty.Register(
            "Target",
            typeof(object),
            typeof(SelectionControlGrid),
            new PropertyMetadata(null));

        static SelectionControlGrid()
        {
            ResDict = new ResourceDictionary { Source = new Uri("WPFTest;component/Pages/BasicPages.res.xaml", UriKind.RelativeOrAbsolute) };
            SelCtrlGridTemplate = (ControlTemplate)ResDict["SelCtrlGridTemplate"];
        }

        public readonly ControllableGrid CtrlGrid;
        private readonly ScrollViewer Scroller;
        public ObservableCollection<FrameworkElement> Children => CtrlGrid.Children;
        public object Target
        {
            get => GetValue(TargetProperty);
            set => SetValue(TargetProperty, value);
        }


        public SelectionControlGrid()
        {
            SetBinding(TargetProperty, new Binding { Source = this, Path = DataContextPath });
            Template = SelCtrlGridTemplate;
            ApplyTemplate();
            CtrlGrid = (ControllableGrid)SelCtrlGridTemplate.FindName("ctl", this);
            Scroller = (ScrollViewer)SelCtrlGridTemplate.FindName("scroll", this);
        }

    }
}
