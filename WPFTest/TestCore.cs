﻿using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Basic3D;
using OpenGLUtil;
using RayRender;
using XZiar.Util;

namespace WPFTest
{
    public enum OPObject { Camera, Drawable, Light };
    public class TestCore : IDisposable
    {
        public class LightList : ObservableList<Light>, IDisposable
        {
            private readonly LightHolder Holder;
            private ushort curLgtIdx;
            public ushort CurLgtIdx
            {
                get { return curLgtIdx; }
                set
                {
                    if (value == ushort.MaxValue)
                        curLgtIdx = (ushort)(Holder.Size - 1);
                    else if (value >= Holder.Size)
                        curLgtIdx = 0;
                    else
                        curLgtIdx = value;
                    OnPropertyChanged("Current");
                }
            }
            public Light Current
            {
                get { return Holder[curLgtIdx]; }
                set { CurLgtIdx = Holder.GetIndex(value); }
            }
            internal LightList(LightHolder holder) : base(holder.Lights)
            {
                Holder = holder;
                curLgtIdx = 0;
                holder.Changed += (s, o) =>
                {
                    if (o == Current)
                        OnPropertyChanged("Current");
                    //OnItemContentChanged(o);
                };
            }
            public void Add(LightType type)
            {
                Holder.Add(type);
                OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Add, Holder[Holder.Size - 1]));
                CurLgtIdx = ushort.MaxValue;
            }
            internal void Refresh()
            {
                Holder.Refresh();
                OnCollectionChanged();
                CurLgtIdx = 0;
            }

            public void Dispose()
            {
                Holder.Container.Clear();
                CurLgtIdx = ushort.MaxValue;
                OnCollectionChanged();
            }
        }
        public class DrawableList : ObservableList<Drawable>, IDisposable
        {
            private readonly DrawableHolder Holder;
            private ushort curObjIdx;
            public ushort CurObjIdx
            {
                get { return curObjIdx; }
                set
                {
                    if (value == ushort.MaxValue)
                        curObjIdx = (ushort)(Holder.Size - 1);
                    else if (value >= Holder.Size)
                        curObjIdx = 0;
                    else
                        curObjIdx = value;
                    OnPropertyChanged("Current");
                }
            }
            public Drawable Current
            {
                get { return Holder[curObjIdx]; }
                set { CurObjIdx = Holder.GetIndex(value); }
            }
            internal DrawableList(DrawableHolder holder) : base(holder.Drawables)
            {
                Holder = holder;
                curObjIdx = 0;
                holder.Changed += (s, o) =>
                {
                    if (o == Current)
                        OnPropertyChanged("Current");
                    //OnItemContentChanged(o);
                };
            }
            public async Task<bool> AddModelAsync(string fileName)
            {
                var ret = await Holder.AddModelAsync(fileName);
                if (ret)
                {
                    OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Add, Holder[Holder.Size - 1]));
                    CurObjIdx = ushort.MaxValue;
                }
                return ret;
            }

            public void Dispose()
            {
                Holder.Container.Clear();
                CurObjIdx = ushort.MaxValue;
                OnCollectionChanged();
            }
        }

        public class ShaderList : ObservableList<GLProgram>, IDisposable
        {
            private readonly ShaderHolder Holder;
            internal ShaderList(ShaderHolder holder) : base(holder.Shaders)
            {
                Holder = holder;
                Current = Holder.GetCurrent();
                holder.Changed += (s, o) =>
                {
                    OnItemContentChanged(o);
                };
            }
            public GLProgram Current
            {
                get;
                private set;
            }
            public async Task<bool> AddShaderAsync(string fileName)
            {
                var shaderName = DateTime.Now.ToString("HH:mm:ss");
                var ret = await Holder.AddShaderAsync(fileName, shaderName);
                if (ret)
                {
                    OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Add, Holder[Holder.Size - 1]));
                }
                return ret;
            }
            public void UseProgram(GLProgram prog)
            {
                Holder.UseShader(prog);
                Current = Holder.GetCurrent();
                OnPropertyChanged("Current");
            }

            public void Dispose()
            {
                Holder.Container.Clear();
                Current = null;
                OnPropertyChanged("Current");
                OnCollectionChanged();
            }
        }

        public readonly BasicTest Test;
        public readonly DrawableList Drawables;
        public readonly LightList Lights;
        public readonly ShaderList Shaders;
        public readonly Camera Camera;

        public bool IsAnimate = false;

        public bool Mode
        {
            get { return Test.Mode; }
            set { Test.Mode = value; }
        }

        public void Save(string fname)
        {
            Test.Serialize(fname);
        }

        public void Load(string fname)
        {
            Test.DeSerialize(fname);
            Lights.Refresh();
        }

        public TestCore()
        {
            Test = new BasicTest();
            Drawables = new DrawableList(Test.Drawables);
            Lights = new LightList(Test.Lights);
            Shaders = new ShaderList(Test.Shaders);
            Camera = Test.Camera;
        }

        public void Move(float x, float y, float z, OPObject obj)
        {
            switch(obj)
            {
            case OPObject.Drawable:
                Drawables.Current?.Move(x, y, z);
                break;
            case OPObject.Camera:
                Test.Camera.Move(x, y, z);
                break;
            case OPObject.Light:
                Lights.Current?.Move(x, y, z);
                break;
            }
        }
        public void Rotate(float x, float y, float z, OPObject obj)
        {
            //conver to radius
            const float muler = (float)(Math.PI / 180);
            x *= muler; y *= muler; z *= muler;
            switch (obj)
            {
            case OPObject.Drawable:
                Drawables.Current?.Rotate(-x, -y, -z);
                break;
            case OPObject.Camera:
                Test.Camera.Rotate(x, y, z);
                break;
            case OPObject.Light:
                Lights.Current?.Rotate(-x, -y, -z);
                break;
            }
        }

        #region IDisposable Support
        private bool disposedValue = false; // 要检测冗余调用

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: 释放托管状态(托管对象)。
                }
                Lights.Dispose();
                Drawables.Dispose();
                Shaders.Dispose();
                Test.Dispose();

                disposedValue = true;
            }
        }

        ~TestCore()
        {
            // 请勿更改此代码。将清理代码放入以上 Dispose(bool disposing) 中。
            Dispose(false);
        }

        // 添加此代码以正确实现可处置模式。
        public void Dispose()
        {
            // 请勿更改此代码。将清理代码放入以上 Dispose(bool disposing) 中。
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        #endregion
    }
}
