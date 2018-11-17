using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Markup;
using System.Windows.Media;
using Common;
using RayRender;
using Xceed.Wpf.Toolkit;
using Xceed.Wpf.Toolkit.PropertyGrid;
using Xceed.Wpf.Toolkit.PropertyGrid.Editors;
using static XZiar.Util.BindingHelper;

namespace WPFTest
{
    public class XCTKPropertyDescriptor : PropertyDescriptor
    {
        internal readonly ControlItem Item;
        public XCTKPropertyDescriptor(ControlItem item, Attribute[] attributes) : base(item.Id, attributes)
        {
            Item = item;
        }

        public override Type ComponentType => typeof(XCTKControllable);

        public override bool IsReadOnly => Item.Access == ControlItem.PropAccess.Read;

        public override Type PropertyType => Item.ValType;

        public override bool CanResetValue(object component) => false;

        public override object GetValue(object component)
        {
            return (component as XCTKControllable).Control.DoGetMember(Item.Id, out object val) ? val : null;
        }

        public override void ResetValue(object component) => throw new NotImplementedException();

        public override void SetValue(object component, object value)
        {
            (component as XCTKControllable).Control.DoSetMember(Item.Id, value);
        }

        public override bool ShouldSerializeValue(object component) => false;
    }

    public class XCTKControllable : ICustomTypeDescriptor, INotifyPropertyChanged
    {
        internal readonly Controllable Control;
        private readonly XCTKPropertyDescriptor[] CustomFields;

        public event PropertyChangedEventHandler PropertyChanged;

        private Attribute[] GenerateAttributes(ControlItem item)
        {
            var attrs = new List<Attribute>
            {
                new DisplayNameAttribute(item.Name),
                new DescriptionAttribute(item.Description)
            };

            if (Control.Categories.TryGetValue(item.Category, out string catName) && !string.IsNullOrEmpty(catName))
                attrs.Add(new CategoryAttribute(catName));
            if (item.Type == ControlItem.PropType.LongText)
                attrs.Add(new EditorAttribute(typeof(ButtonTextEditor), typeof(ButtonTextEditor)));
            if (item.ValType == typeof(string) && item.Cookie is string[] choices)
                attrs.Add(new EditorAttribute(typeof(EnumStringEditor), typeof(EnumStringEditor)));
            if (item.ValType == typeof(Vector2))
                attrs.Add(new EditorAttribute(typeof(Range2Editor), typeof(Range2Editor)));
            if (item.ValType == typeof(float))
                attrs.Add(new EditorAttribute(typeof(RangeEditor), typeof(RangeEditor)));
            return attrs.ToArray();
        }
        public XCTKControllable(Controllable control)
        {
            Control = control;
            Control.PropertyChanged += (o, e) => PropertyChanged?.Invoke(this, e);
            CustomFields = Control.Items.Select(item => new XCTKPropertyDescriptor(item, GenerateAttributes(item))).ToArray();
        }
        public override string ToString()
        {
            if (Control.DoGetMember("Name", out object name))
                return $"[{Control.ControlType}]{name}";
            else
                return $"[{Control.ControlType}]";
        }
        #region Hide not implemented members
        public AttributeCollection GetAttributes()
        {
            return null;
        }

        public string GetClassName()
        {
            return GetType().Name;
        }

        public string GetComponentName()
        {
            if (Control.DoGetMember("Name", out object name))
                return name as string;
            else
                return Control.ControlType;
        }

        public TypeConverter GetConverter()
        {
            return null;
        }

        public EventDescriptor GetDefaultEvent()
        {
            return null;
        }

        public PropertyDescriptor GetDefaultProperty()
        {
            return null;
        }

        public object GetEditor(Type editorBaseType)
        {
            return null;
        }

        public EventDescriptorCollection GetEvents()
        {
            return null;
        }

        public EventDescriptorCollection GetEvents(Attribute[] attributes)
        {
            return null;
        }

        public object GetPropertyOwner(PropertyDescriptor pd)
        {
            return this;
        }
        #endregion
        public PropertyDescriptorCollection GetProperties()
        {
            return new PropertyDescriptorCollection(CustomFields);
        }

        public PropertyDescriptorCollection GetProperties(Attribute[] attributes)
        {
            return new PropertyDescriptorCollection(CustomFields);
        }
    }

    public class EnumStringEditor : EnumComboBoxEditor
    {
        protected override IEnumerable CreateItemsSource(PropertyItem propertyItem)
        {
            return (propertyItem.PropertyDescriptor as XCTKPropertyDescriptor).Item.Cookie as string[];
        }
    }
    public class ButtonTextEditor : ITypeEditor
    {
        public FrameworkElement ResolveEditor(PropertyItem propertyItem)
        {
            var btn = new Button
            {
                Content = "查看"
            };
            btn.Click += (o, e) =>
            {
                new TextDialog(propertyItem.Value as string, $"{(propertyItem.Instance as XCTKControllable).ToString()} --- {propertyItem.PropertyDescriptor.Name}").Show();
                e.Handled = true;
            };
            return btn;
        }
    }
    public class RangeEditor : ITypeEditor
    {
        static readonly TwoWayValueConvertor Convertor = TwoWayValueConvertor.From(Convert.ToSingle);
        public FrameworkElement ResolveEditor(PropertyItem propertyItem)
        {
            var limit = (Vector2)(propertyItem.PropertyDescriptor as XCTKPropertyDescriptor).Item.Cookie;
            var slider = new Slider
            {
                AutoToolTipPlacement = AutoToolTipPlacement.TopLeft,
                AutoToolTipPrecision = 3,
                Minimum = limit.X,
                Maximum = limit.Y
            };

            slider.SetBinding(Slider.ValueProperty, new Binding("Value")
            {
                Source = propertyItem,
                Mode = BindingMode.TwoWay,
                Converter = Convertor
            });
            return slider;
        }
    }
    public class Range2Editor : ITypeEditor
    {
        public FrameworkElement ResolveEditor(PropertyItem propertyItem)
        {
            var limit = (Vector2)(propertyItem.PropertyDescriptor as XCTKPropertyDescriptor).Item.Cookie;
            var slider = new RangeSlider
            {
                AutoToolTipPlacement = AutoToolTipPlacement.TopLeft,
                AutoToolTipPrecision = 3,
                Minimum = limit.X,
                Maximum = limit.Y
            };

            slider.SetBinding(RangeSlider.LowerValueProperty, new Binding("Value")
            {
                Source = propertyItem,
                Mode = BindingMode.TwoWay,
                Converter = new TwoWayValueConvertor(
                    o => ((Vector2)o).X,
                    o => new Vector2(Convert.ToSingle(o), (float)slider.HigherValue))
            });
            slider.SetBinding(RangeSlider.HigherValueProperty, new Binding("Value")
            {
                Source = propertyItem,
                Mode = BindingMode.TwoWay,
                Converter = new TwoWayValueConvertor(
                    o => ((Vector2)o).Y, 
                    o => new Vector2((float)slider.LowerValue, Convert.ToSingle(o)))
            });
            return slider;
        }
    }

}