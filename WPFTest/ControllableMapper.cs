using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Markup;
using System.Windows.Media;
using RayRender;
using Xceed.Wpf.Toolkit.PropertyGrid;
using Xceed.Wpf.Toolkit.PropertyGrid.Editors;

namespace WPFTest
{
    public class XCTKPropertyDescriptor : PropertyDescriptor
    {
        internal readonly ControlItem Item;
        public XCTKPropertyDescriptor(ControlItem item) : base(item.Id, GenerateAttributes(item))
        {
            Item = item;
        }
        private static Attribute[] GenerateAttributes(ControlItem item)
        {
            var attrs = new List<Attribute>
            {
                new DisplayNameAttribute(item.Name),
                new DescriptionAttribute(item.Description)
            };

            if (!string.IsNullOrEmpty(item.Category))
                attrs.Add(new CategoryAttribute(item.Category));
            if (item.ValType == typeof(string) && item.Cookie is string[] choices)
            {
                attrs.Add(new EditorAttribute(typeof(EnumStringEditor), typeof(EnumStringEditor)));
            }
            return attrs.ToArray();
        }
        //public override string Category { get => Item.Category; }

        public override Type ComponentType => typeof(XCTKControllable);

        public override bool IsReadOnly => Item.Access == ControlItem.PropAccess.Read;

        public override Type PropertyType => Item.ValType;

        public override bool CanResetValue(object component) => false;

        public override object GetValue(object component)
        {
            return (component as XCTKControllable).DoGetMember(Item.Id, out object val) ? val : null;
        }

        public override void ResetValue(object component) => throw new NotImplementedException();

        public override void SetValue(object component, object value)
        {
            (component as XCTKControllable).DoSetMember(Item.Id, value);
        }

        public override bool ShouldSerializeValue(object component) => false;
    }

    public class XCTKControllable : Controllable, ICustomTypeDescriptor
    {
        private readonly List<XCTKPropertyDescriptor> CustomFields;
        public XCTKControllable(Controllable control) : base(control)
        {
            CustomFields = Items.Select(item => new XCTKPropertyDescriptor(item)).ToList();
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
            return Name;
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
            return new PropertyDescriptorCollection(CustomFields.ToArray());
        }

        public PropertyDescriptorCollection GetProperties(Attribute[] attributes)
        {
            return new PropertyDescriptorCollection(CustomFields.ToArray());
        }
    }

    public class EnumStringEditor : EnumComboBoxEditor
    {
        protected override IEnumerable CreateItemsSource(PropertyItem propertyItem)
        {
            return (propertyItem.PropertyDescriptor as XCTKPropertyDescriptor).Item.Cookie as string[];
        }
    }

    public static class ControllableMapper
    {
        private static readonly BrushConverter BrushConv = new BrushConverter();
        private static readonly SolidColorBrush bgBrush = new SolidColorBrush(Colors.White);
        private static readonly SolidColorBrush borderBrush = BrushConv.ConvertFromString("#FFABABAB") as SolidColorBrush;
        static StackPanel GetControlPanel(Controllable control)
        {
            var panel = new StackPanel();
            foreach (var cat in control.Categories)
            {
                IAddChild parent = panel;
                if (!string.IsNullOrEmpty(cat.Key))
                {
                    var group = new GroupBox()
                    {
                        Header = cat.Value,
                        Foreground = bgBrush,
                        BorderBrush = borderBrush,
                        BorderThickness = new System.Windows.Thickness(0.5),
                        Margin = new System.Windows.Thickness(3, 4, 3, 4)
                    };
                    panel.Children.Add(group);
                    parent = group;
                }
                foreach (var item in control.Items.Where(x => x.Category == cat.Key))
                {

                }
            }
            return null;
        }

    }

}