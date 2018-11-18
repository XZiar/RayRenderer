using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;
using System.Windows.Markup;

namespace CommonUtil
{
    public class ProxyBinder : DependencyObject
    {
        public static readonly DependencyProperty DestSlotProperty = DependencyProperty.Register(
            "DestSlot",
            typeof(object),
            typeof(ProxyBinder),
            new PropertyMetadata(null));
        private static readonly DependencyProperty SrcSlotProperty = DependencyProperty.Register(
            "SrcSlot",
            typeof(object),
            typeof(ProxyBinder),
            new PropertyMetadata(null, SourceChanged));
        public static readonly DependencyProperty DataSourceProperty = DependencyProperty.Register(
            "DataSource",
            typeof(object),
            typeof(ProxyBinder),
            new PropertyMetadata(null, DataSourceChanged));

        private readonly Func<object, object> Converter;
        private readonly PropertyPath Path;
        private int UpdateCount = 0;

        public ProxyBinder(PropertyPath path, Func<object, object> converter)
        {
            Path = path; Converter = converter;
        }
        public ProxyBinder(Binding binding, Func<object, object> converter)
        {
            BindingOperations.SetBinding(this, SrcSlotProperty, binding); Converter = converter;
        }

        public Binding GetDestBinding()
        {
            return new Binding
            {
                Path = new PropertyPath(DestSlotProperty),
                Source = this,
                Mode = BindingMode.OneWay,
            };
        }

        static void DataSourceChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (!(d is ProxyBinder self))
                return;
            if (e.NewValue == null)
                self.SetValue(SrcSlotProperty, null);
            else
                BindingOperations.SetBinding(self, SrcSlotProperty, new Binding
                {
                    Path = self.Path,
                    Source = e.NewValue,
                    Mode = BindingMode.OneWay,
                });
        }
        static async void SourceChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (!(d is ProxyBinder self))
                return;
            if (e.NewValue == e.OldValue)
                return;
            var result = self.Converter(e.NewValue);
            var id = Interlocked.Increment(ref self.UpdateCount);
            if (result.GetType().GetGenericTypeDefinition() == typeof(Task<>))
            {
                object obj = await (result as dynamic);
                if (id == Interlocked.CompareExchange(ref self.UpdateCount, 0, 0))
                    self.SetValue(DestSlotProperty, obj);
            }
            else
            {
                self.SetValue(DestSlotProperty, result);
            }
        }
    }
}
