using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Common;
using Xceed.Wpf.Toolkit;
using static XZiar.Util.BindingHelper;

namespace WPFTest
{
    /// <summary>
    /// ControllableGrid.xaml 的交互逻辑
    /// </summary>
    public partial class ControllableGrid : UserControl
    {
        public delegate void GenerateControlEventHandler(ControllableGrid sender, ControlItem item, ref FrameworkElement element);
        public event GenerateControlEventHandler GeneratingControl;
        public ControllableGrid()
        {
            InitializeComponent();
            DataContextChanged += OnDataContextChanged;
            BindingForeground = new Binding
            {
                Source = this,
                Path = new PropertyPath("Foreground"),
                Mode = BindingMode.OneWay
            };
            BindingBackground = new Binding
            {
                Source = this,
                Path = new PropertyPath("Background"),
                Mode = BindingMode.OneWay
            };
            BindingBorderBrush = new Binding
            {
                Source = this,
                Path = new PropertyPath("BorderBrush"),
                Mode = BindingMode.OneWay
            };
        }
        private readonly Binding BindingForeground;
        private readonly Binding BindingBackground;
        private readonly Binding BindingBorderBrush;
        private static readonly TwoWayValueConvertor ConvertorForceSingle = TwoWayValueConvertor.From(Convert.ToSingle);
        private static readonly BrushConverter BrushConv = new BrushConverter();
        private static readonly SolidColorBrush BrushBorder = BrushConv.ConvertFromString("#FFABABAB") as SolidColorBrush;
        private Controllable GetControllable() { return DataContext as Controllable; }
        public Binding CreateBinding(ControlItem item, IValueConverter convertor = null)
        {
            return new Binding(item.Id)
            {
                Source = DataContext,
                Mode = item.Access.HasFlag(ControlItem.PropAccess.Write) ? BindingMode.TwoWay : BindingMode.OneWay,
                Converter = convertor
            };
        }

        private void OnDataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (!(e.NewValue is Controllable control))
            {
                stkMain.DataContext = null;
                stkMain.ItemsSource = null;
                return;
            }
            stkMain.DataContext = control;
            stkMain.ItemsSource = control.Categories;
        }

        private static FrameworkElement OnGeneratingControl(ControllableGrid sender, ControlItem item)
        {
            FrameworkElement result = null;
            sender.GeneratingControl?.Invoke(sender, item, ref result);
            if (result != null) return result;
            if(item.Type == ControlItem.PropType.Enum)
            {
                //if (item.ValType == typeof(string))
                {
                    var cbox = new ComboBox
                    {
                        ItemsSource = item.Cookie as string[]
                    };
                    cbox.SetBinding(ComboBox.SelectedItemProperty, sender.CreateBinding(item));
                    return cbox;
                }

            }
            if (item.ValType == typeof(string))
            {
                if (item.Type == ControlItem.PropType.LongText)
                {
                    var btn = new Button { Content = "查看" };
                    btn.Click += (o, e) =>
                    {
                        var control = sender.GetControllable();
                        control.DoGetMember(item.Id, out object txt);
                        new TextDialog(txt as string, $"{control.ToString()} --- {item.Name}").Show();
                        e.Handled = true;
                    };
                    return btn;
                }
                {
                    var tbox = new TextBox
                    {
                        VerticalAlignment = VerticalAlignment.Center
                    };
                    tbox.SetBinding(TextBox.TextProperty, sender.CreateBinding(item));
                    tbox.IsReadOnly = !item.Access.HasFlag(ControlItem.PropAccess.Write);
                    return tbox;
                }
            }
            if (item.ValType == typeof(Vector2))
            {
                var limit = (Vector2)item.Cookie;
                var slider = new RangeSlider
                {
                    AutoToolTipPlacement = AutoToolTipPlacement.TopLeft,
                    AutoToolTipPrecision = 3,
                    Minimum = limit.X,
                    Maximum = limit.Y
                };
                slider.SetBinding(RangeSlider.LowerValueProperty,
                    sender.CreateBinding(item, new TwoWayValueConvertor(
                        o => ((Vector2)o).X,
                        o => new Vector2(Convert.ToSingle(o), (float)slider.HigherValue))
                        ));
                slider.SetBinding(RangeSlider.HigherValueProperty,
                    sender.CreateBinding(item, new TwoWayValueConvertor(
                        o => ((Vector2)o).Y,
                        o => new Vector2((float)slider.LowerValue, Convert.ToSingle(o)))
                        ));
                return slider;
            }
            if (item.ValType == typeof(Color))
            {
                var clrPicker = new ColorPicker()
                {
                    ColorMode = ColorMode.ColorCanvas
                };
                clrPicker.SetBinding(ColorPicker.SelectedColorProperty, sender.CreateBinding(item));
                return clrPicker;
            }
            if (item.ValType == typeof(float))
            {
                if(item.Cookie == null)
                {
                    var spiner = new DoubleUpDown { FormatString = "F2", Increment = 0.1 };
                    spiner.SetBinding(DoubleUpDown.ValueProperty, sender.CreateBinding(item, ConvertorForceSingle));
                    return spiner;
                }
                else
                {
                    var limit = (Vector2)item.Cookie;
                    var slider = new Slider
                    {
                        AutoToolTipPlacement = AutoToolTipPlacement.TopLeft,
                        AutoToolTipPrecision = 3,
                        Minimum = limit.X,
                        Maximum = limit.Y
                    };
                    slider.SetBinding(Slider.ValueProperty, sender.CreateBinding(item, ConvertorForceSingle));
                    return slider;
                }
            }
            if (item.ValType == typeof(bool))
            {
                var ckBox = new CheckBox
                {
                    VerticalAlignment = VerticalAlignment.Center,
                    HorizontalAlignment = HorizontalAlignment.Center
                };
                ckBox.SetBinding(CheckBox.IsCheckedProperty, sender.CreateBinding(item));
                return ckBox;
            }
            {
                var tblock = new TextBlock
                {
                    VerticalAlignment = VerticalAlignment.Center,
                    HorizontalAlignment = HorizontalAlignment.Center
                };
                tblock.SetBinding(TextBlock.TextProperty, sender.CreateBinding(item));
                return tblock;
            }
        }

        private void SetBasicBinding(FrameworkElement element)
        {
            element.Margin = new Thickness(6, 4, 6, 4);
            switch (element)
            {
            case Panel panel:
                panel.SetBinding(Panel.BackgroundProperty, BindingBackground);
                break;
            case TextBlock tblock:
                tblock.SetBinding(TextBlock.BackgroundProperty, BindingBackground);
                tblock.SetBinding(TextBlock.ForegroundProperty, BindingForeground);
                break;
            case Control control:
                if (!(control is CheckBox))
                {
                    control.SetBinding(Control.BackgroundProperty, BindingBackground);
                    control.SetBinding(Control.ForegroundProperty, BindingForeground);
                    //control.SetBinding(Control.BorderBrushProperty, BindingBorderBrush);
                }
                break;
            }
        }
        private void gridMain_Initialized(object sender, EventArgs e)
        {
            var control = GetControllable();
            if (control == null)
                return;
            var grid = sender as Grid;
            var cat = grid.Tag as string;
            var items = control.Items.Where(x => x.Category == cat).ToArray();
            grid.RowDefinitions.Clear();
            for (var i = 0; i < items.Length; ++i)
                grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            int idx = 0;
            foreach (var item in items)
            {
                var label = new TextBlock()
                {
                    Text = item.Name, ToolTip = item.Description,
                    Padding = new Thickness(0, 0, 6, 0),
                    VerticalAlignment = VerticalAlignment.Center, HorizontalAlignment = HorizontalAlignment.Center
                };
                SetBasicBinding(label);
                Grid.SetRow(label, idx); Grid.SetColumn(label, 0);
                grid.Children.Add(label);
                var realControl = OnGeneratingControl(this, item);
                SetBasicBinding(realControl);
                Grid.SetRow(realControl, idx); Grid.SetColumn(realControl, 2);
                grid.Children.Add(realControl);
                idx++;
            }
        }
    }
}
