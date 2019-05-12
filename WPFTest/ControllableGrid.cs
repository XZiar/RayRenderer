using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
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
using XZiar.Util.Collections;
using static XZiar.Util.BindingHelper;

namespace WPFTest
{

    [ContentProperty(nameof(Children))]
    public class ControllableGrid : ContentControl
    {
        private static readonly ResourceDictionary ResDict;
        private static readonly ItemsPanelTemplate ItemsPanelTemplate;
        private static readonly DataTemplate ItemsDataTemplate;
        static ControllableGrid()
        {
            ResDict = new ResourceDictionary { Source = new Uri("WPFTest;component/ControllableGrid.res.xaml", UriKind.RelativeOrAbsolute) };
            ItemsPanelTemplate = (ItemsPanelTemplate)ResDict["ItemsPanelTemplate"];
            ItemsDataTemplate = (DataTemplate)ResDict["ItemsDataTemplate"];
        }

        private class ControlGroupsControl : ItemsControl
        {
            private readonly ControllableGrid Upper;
            internal ControlGroupsControl(ControllableGrid upper)
            {
                Upper = upper;
                ItemsPanel = ItemsPanelTemplate;
                //ItemTemplate = ItemsDataTemplate;
            }
            private static readonly Brush BorderBrush_ = new SolidColorBrush(Color.FromRgb(0xab, 0xab, 0xab));
            private static readonly Thickness BorderThickness_ = new Thickness(0.5);
            private static readonly Thickness Margin_ = new Thickness(3, 4, 3, 4);
            protected override DependencyObject GetContainerForItemOverride()
            {
                return new GroupBox() { BorderBrush = BorderBrush_, BorderThickness = BorderThickness_, Margin = Margin_ };
            }
            protected override void PrepareContainerForItemOverride(DependencyObject element, object item)
            {
                var ele = (GroupBox)element;
                var dat = (KeyValuePair<string, string>)item;
                ele.Header = dat.Value;
                var grid = (Grid)ResDict["theGrid"];
                ele.Content = grid;
                grid.Tag = dat.Key;
                Upper.gridMain_Initialized(grid);
            }
        }


        public static readonly DependencyProperty ItemCategoryProperty = DependencyProperty.RegisterAttached(
            "ItemCategory",
            typeof(string),
            typeof(ControllableGrid),
            new FrameworkPropertyMetadata("", FrameworkPropertyMetadataOptions.AffectsRender));
        public static void SetItemCategory(UIElement element, string value)
        {
            element.SetValue(ItemCategoryProperty, value);
        }
        public static string GetItemCategory(UIElement element)
        {
            return element.GetValue(ItemCategoryProperty) as string;
        }

        public static readonly DependencyProperty ItemNameProperty = DependencyProperty.RegisterAttached(
            "ItemName",
            typeof(string),
            typeof(ControllableGrid),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsRender));
        public static void SetItemName(UIElement element, string value)
        {
            element.SetValue(ItemNameProperty, value);
        }
        public static string GetItemName(UIElement element)
        {
            return element.GetValue(ItemNameProperty) as string;
        }

        public static readonly DependencyProperty ItemDescriptionProperty = DependencyProperty.RegisterAttached(
            "ItemDescription",
            typeof(string),
            typeof(ControllableGrid),
            new FrameworkPropertyMetadata("", FrameworkPropertyMetadataOptions.AffectsRender));
        public static void SetItemDescription(UIElement element, string value)
        {
            element.SetValue(ItemDescriptionProperty, value);
        }
        public static string GetItemDescription(UIElement element)
        {
            return element.GetValue(ItemDescriptionProperty) as string;
        }

        public ObservableCollection<FrameworkElement> Children { get; } = new ObservableCollection<FrameworkElement>();

        //public static readonly DependencyProperty CategoriesProperty = DependencyProperty.Register(
        //    nameof(Categories),
        //    typeof(Dictionary<string, string>),
        //    typeof(ControllableGrid),
        //    new PropertyMetadata());
        //public Dictionary<string, string> Categories
        //{
        //    get { return (Dictionary<string, string>)GetValue(CategoriesProperty); }
        //    set { SetValue(CategoriesProperty, value); }
        //}

        public delegate void GenerateControlEventHandler(ControllableGrid sender, ControlItem item, ref FrameworkElement element);
        public event GenerateControlEventHandler GeneratingControl;
        public ObservableSet<string> ExceptIds { get; } = new ObservableSet<string>();

        private readonly ItemsControl stkMain;

        private readonly Dictionary<string, string> Categories = new Dictionary<string, string>();
        private readonly List<Grid> CategoryGrids = new List<Grid>();


        public ControllableGrid()
        {
            stkMain = new ControlGroupsControl(this);
            Content = stkMain;
            ExceptIds.CollectionChanged += ExceptIdsChanged;
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

        private void ExceptIdsChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            Refresh();
        }

        private readonly Binding BindingForeground;
        private readonly Binding BindingBackground;
        private readonly Binding BindingBorderBrush;
        private static readonly TwoWayValueConvertor ConvertorForceUShort = TwoWayValueConvertor.From(Convert.ToUInt16);
        private static readonly TwoWayValueConvertor ConvertorForceInt    = TwoWayValueConvertor.From(Convert.ToInt32);
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
            Refresh();
        }

        private void Refresh()
        {
            foreach (var grid in CategoryGrids)
                grid.Children.Clear();
            CategoryGrids.Clear();

            if (!(DataContext is Controllable control))
            {
                stkMain.DataContext = null;
                stkMain.ItemsSource = null;
                return;
            }

            stkMain.DataContext = control;
            foreach (var element in Children)
                element.DataContext = control;

            var newcat = Children.Select(x => GetItemCategory(x))
                .Where(x => !control.Categories.ContainsKey(x))
                .Select(x => new KeyValuePair<string, string>(x, x));
            var allcat = control.Categories.Concat(newcat)
                .Where(x => !Categories.ContainsKey(x.Key));

            stkMain.ItemsSource = Categories.Concat(allcat);
        }

        private void SetBasicBinding(FrameworkElement element)
        {
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
        private class ControlPack
        {
            public string Name;
            public string Description;
            public FrameworkElement Control;
            public ControlPack(ControlItem item, FrameworkElement control)
            {
                Name = item.Name; Description = item.Description; Control = control;
            }
            public ControlPack(FrameworkElement control)
            {
                Name = GetItemName(control); Description = GetItemDescription(control); Control = control;
            }
        }
        private void gridMain_Initialized(Grid grid)
        {
            var control = GetControllable();
            if (control == null)
                return;
            CategoryGrids.Add(grid);
            var splitter = (GridSplitter)grid.Children[0];
            var cat = grid.Tag as string;

            var itemControls = control.Items
                .Where(x => x.Category == cat && !ExceptIds.Contains(x.Id))
                .Select(item =>
                {
                    var realControl = OnGeneratingControl(item);
                    SetBasicBinding(realControl);
                    SetItemCategory(realControl, cat);
                    return new ControlPack(item, realControl);
                });

            var manualControls = Children.Where(x => GetItemCategory(x) == cat)
                .Select(element => new ControlPack(element));

            var controls = itemControls.Concat(manualControls);

            grid.RowDefinitions.Clear();
            for (var i = 0; i < controls.Count(); ++i)
                grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            int idx = 0, idxEnd = controls.Count() - 1;
            foreach (var pack in controls)
            {
                var realControl = pack.Control;
                realControl.Margin = new Thickness(6, 4, 6, 4);
                bool setTooltip = !string.IsNullOrWhiteSpace(pack.Description);
                if (string.IsNullOrWhiteSpace(pack.Name))
                {
                    if (setTooltip) 
                        realControl.ToolTip = pack.Description;
                    Grid.SetRow(realControl, idxEnd); Grid.SetColumn(realControl, 0); Grid.SetColumnSpan(realControl, 3);
                    grid.Children.Add(realControl);
                    idxEnd--;
                }
                else
                {
                    var label = new TextBlock()
                    {
                        Text = pack.Name,
                        Padding = new Thickness(0, 0, 6, 0),
                        VerticalAlignment = VerticalAlignment.Center,
                        HorizontalAlignment = HorizontalAlignment.Center
                    };
                    if (setTooltip)
                        label.ToolTip = pack.Description;
                    label.SetBinding(VisibilityProperty, new Binding
                    {
                        Source = realControl,
                        Path = new PropertyPath(VisibilityProperty),
                        Mode = BindingMode.OneWay
                    });
                    SetBasicBinding(label);

                    Grid.SetRow(label, idx); Grid.SetColumn(label, 0);
                    grid.Children.Add(label);
                    Grid.SetRow(realControl, idx); Grid.SetColumn(realControl, 2);
                    grid.Children.Add(realControl);
                    idx++;
                }
            }
            if (idx > 0)
            {
                Grid.SetRowSpan(splitter, idx);
                splitter.Visibility = Visibility.Visible;
            }
            else
                splitter.Visibility = Visibility.Collapsed;
        }

        private FrameworkElement OnGeneratingControl(ControlItem item)
        {
            FrameworkElement result = null;
            GeneratingControl?.Invoke(this, item, ref result);
            if (result != null) return result;
            if (item.Type == ControlItem.PropType.Enum)
            {
                var cbox = new ComboBox
                {
                    ItemsSource = item.Cookie as string[]
                };
                cbox.SetBinding(ComboBox.SelectedItemProperty, this.CreateBinding(item));
                return cbox;
            }
            if (item.ValType == typeof(string))
            {
                if (item.Type == ControlItem.PropType.LongText)
                {
                    var btn = new Button { Content = "查看" };
                    btn.Click += (o, e) =>
                    {
                        var control = GetControllable();
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
                    tbox.SetBinding(TextBox.TextProperty, CreateBinding(item));
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
                    CreateBinding(item, new TwoWayValueConvertor(
                        o => ((Vector2)o).X,
                        o => new Vector2(Convert.ToSingle(o), (float)slider.HigherValue))
                        ));
                slider.SetBinding(RangeSlider.HigherValueProperty,
                    CreateBinding(item, new TwoWayValueConvertor(
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
                clrPicker.SetBinding(ColorPicker.SelectedColorProperty, CreateBinding(item));
                return clrPicker;
            }
            if (item.ValType == typeof(float))
            {
                if (item.Cookie == null)
                {
                    var spiner = new DoubleUpDown { FormatString = "F2", Increment = 0.1 };
                    spiner.SetBinding(DoubleUpDown.ValueProperty, CreateBinding(item, ConvertorForceSingle));
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
                    slider.SetBinding(Slider.ValueProperty, CreateBinding(item, ConvertorForceSingle));
                    return slider;
                }
            }
            if (item.ValType == typeof(ushort))
            {
                if (item.Cookie == null)
                {
                    var spiner = new IntegerUpDown { Increment = 1 };
                    spiner.SetBinding(IntegerUpDown.ValueProperty, CreateBinding(item, ConvertorForceUShort));
                    return spiner;
                }
                else
                {
                    dynamic limit = item.Cookie;
                    var slider = new Slider
                    {
                        AutoToolTipPlacement = AutoToolTipPlacement.TopLeft,
                        Minimum = limit.Item1,
                        Maximum = limit.Item2
                    };
                    slider.SetBinding(Slider.ValueProperty, CreateBinding(item, ConvertorForceUShort));
                    return slider;
                }
            }
            if (item.ValType == typeof(int))
            {
                if (item.Cookie == null)
                {
                    var spiner = new IntegerUpDown { Increment = 1 };
                    spiner.SetBinding(IntegerUpDown.ValueProperty, CreateBinding(item));
                    return spiner;
                }
                else
                {
                    dynamic limit = item.Cookie;
                    var slider = new Slider
                    {
                        AutoToolTipPlacement = AutoToolTipPlacement.TopLeft,
                        Minimum = limit.Item1,
                        Maximum = limit.Item2
                    };
                    slider.SetBinding(Slider.ValueProperty, CreateBinding(item, ConvertorForceInt));
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
                ckBox.SetBinding(CheckBox.IsCheckedProperty, CreateBinding(item));
                return ckBox;
            }
            {
                var tblock = new TextBlock
                {
                    VerticalAlignment = VerticalAlignment.Center,
                    HorizontalAlignment = HorizontalAlignment.Center
                };
                tblock.SetBinding(TextBlock.TextProperty, CreateBinding(item));
                return tblock;
            }
        }

    }
}
