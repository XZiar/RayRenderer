using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;
using System.Windows.Markup;

namespace CommonUtil
{
    public class ProxyBinder : DependencyObject, INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        protected object output;
        private object oldInput;
        public object Output
        {
            get => output;
            set
            {
                if (output == value) return;
                OutputChanged(value);
                output = value;
            }
        }
        private static readonly DependencyProperty InputProperty = DependencyProperty.Register(
            "Input",
            typeof(object),
            typeof(ProxyBinder),
            new PropertyMetadata(null, InputChanged));
        public static readonly DependencyProperty DataSourceProperty = DependencyProperty.Register(
            "DataSource",
            typeof(object),
            typeof(ProxyBinder),
            new PropertyMetadata(null, DataSourceChanged));

        public Func<object, object> Converter;
        public Func<object, object> ConverterBack;
        private readonly PropertyPath Path;
        private readonly BindingMode Mode;
        private int UpdateCount = 0;


        public ProxyBinder(PropertyPath path, BindingMode mode, Func<object, object> converter = null, Func<object, object> converterBack = null)
        {
            Path = path; Mode = mode; Converter = converter; ConverterBack = converterBack;
        }
        public ProxyBinder(Binding binding, Func<object, object> converter = null, Func<object, object> converterBack = null)
        {
            Mode = binding.Mode; BindingOperations.SetBinding(this, InputProperty, binding); Converter = converter; ConverterBack = converterBack;
        }

        public Binding GetDestBinding()
        {
            return new Binding
            {
                Path = new PropertyPath("Output"),
                Source = this,
                Mode = Mode,
            };
        }

        void UpdateInput(object newVal, int id)
        {
            if (id == Interlocked.CompareExchange(ref UpdateCount, 0, 0))
            {
                oldInput = newVal;
                SetValue(InputProperty, newVal);
            }
        }
        void UpdateOutput(object newVal, int id)
        {
            if (id == Interlocked.CompareExchange(ref UpdateCount, 0, 0))
            {
                output = newVal;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Output)));
            }
        }

        async void OutputChanged(object newVal)
        {
            var result = ConverterBack == null ? newVal : ConverterBack(newVal);
            var id = Interlocked.Increment(ref UpdateCount);
            if (result.GetType().GetGenericTypeDefinition() == typeof(Task<>))
            {
                object obj = await (result as dynamic);
                UpdateInput(obj, id);
            }
            else
            {
                UpdateInput(result, id);
            }
        }
        static async void InputChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (!(d is ProxyBinder self))
                return;
            if (e.NewValue == e.OldValue || e.NewValue == self.oldInput) // no change or back-change
                return;
            var result = self.Converter == null ? e.NewValue : self.Converter(e.NewValue);
            var id = Interlocked.Increment(ref self.UpdateCount);
            if (result.GetType().GetGenericTypeDefinition() == typeof(Task<>))
            {
                object obj = await (result as dynamic);
                self.UpdateOutput(obj, id);
            }
            else
            {
                self.UpdateOutput(result, id);
            }
        }
        static void DataSourceChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (!(d is ProxyBinder self))
                return;
            if (e.NewValue == null)
                self.SetValue(InputProperty, null);
            else
                BindingOperations.SetBinding(self, InputProperty, new Binding
                {
                    Path = self.Path,
                    Source = e.NewValue,
                    Mode = self.Mode,
                });
        }
    }
}
