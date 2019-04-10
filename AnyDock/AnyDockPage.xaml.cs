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
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace AnyDock
{
    /// <summary>
    /// DockElement.xaml 的交互逻辑
    /// </summary>
    public partial class AnyDockPage : ContentControl
    {
        public static readonly DependencyProperty PageNameProperty = DependencyProperty.RegisterAttached(
            "PageName",
            typeof(string),
            typeof(AnyDockPage),
            new FrameworkPropertyMetadata("", FrameworkPropertyMetadataOptions.AffectsRender));

        public static void SetPageName(UIElement element, string value)
        {
            element.SetValue(PageNameProperty, value);
        }
        public static string GetPageName(UIElement element)
        {
            return element.GetValue(PageNameProperty) as string;
        }
        public static readonly DependencyProperty HeaderProperty = DependencyProperty.Register(
            nameof(Header),
            typeof(string),
            typeof(AnyDockPanel),
            new PropertyMetadata());
        private string Header
        {
            get { return (string)GetValue(HeaderProperty); }
            set { SetValue(HeaderProperty, value); }
        }

        internal readonly AnyDockPanel ParentPanel;
        public AnyDockPage(AnyDockPanel panel)
        {
            ParentPanel = panel;
            InitializeComponent();
        }
        protected override void OnContentChanged(object oldContent, object newContent)
        {
            if (newContent is AnyDockPage)
                return;
            base.OnContentChanged(oldContent, newContent);
            BindingOperations.ClearBinding(this, HeaderProperty);
            SetBinding(HeaderProperty, new Binding
            { Mode = BindingMode.TwoWay, Source = newContent, Path = new PropertyPath(PageNameProperty) });
        }

        protected override void OnMouseMove(MouseEventArgs e)
        {
            base.OnMouseMove(e);
            if (e.LeftButton == MouseButtonState.Pressed)
            {
                // Package the data.
                DataObject data = new DataObject();
                data.SetData("DockPage", this);
                data.SetData("Keys", Keyboard.Modifiers);

                // Inititate the drag-and-drop operation.
                DragDrop.DoDragDrop(this, data, DragDropEffects.Copy | DragDropEffects.Move);
            }
        }
    }
}
