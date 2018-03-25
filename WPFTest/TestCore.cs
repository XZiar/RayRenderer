using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Basic3D;
using RayRender;

namespace WPFTest
{
    public enum OPObject { Camera, Drawable, Light };
    public class TestCore : INotifyPropertyChanged, IDisposable
    {
        public readonly BasicTest Test;

        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private ushort curObjIdx;
        public ushort CurObjIdx
        {
            get { return curObjIdx; }
            set
            {
                if (value == ushort.MaxValue)
                    curObjIdx = (ushort)(Test.Drawables.Size - 1);
                else if(value >= Test.Drawables.Size)
                    curObjIdx = 0;
                else
                    curObjIdx = value;
                OnPropertyChanged("CurObj");
            }
        }
        public Drawable CurObj
        {
            get { return Test.Drawables[curObjIdx]; }
        }

        private ushort curLgtIdx;
        public ushort CurLgtIdx
        {
            get { return curLgtIdx; }
            set
            {
                if (value == ushort.MaxValue)
                    curLgtIdx = (ushort)(Test.Lights.Size - 1);
                else if (value >= Test.Lights.Size)
                    curLgtIdx = 0;
                else
                    curLgtIdx = value;
                OnPropertyChanged("CurLgt");
            }
        }
        public Light CurLgt
        {
            get { return Test.Lights[curLgtIdx]; }
        }

        public bool IsAnimate = false;

        public bool Mode
        {
            get { return Test.Mode; }
            set { Test.Mode = value; }
        }


        public TestCore()
        {
            Test = new BasicTest();
            CurObjIdx = 0; CurLgtIdx = 0;
        }

        public void Move(float x, float y, float z, OPObject obj)
        {
            switch(obj)
            {
            case OPObject.Drawable:
                Test.Moveobj(curObjIdx, x, y, z);
                break;
            case OPObject.Camera:
                Test.Camera.Move(x, y, z);
                break;
            case OPObject.Light:
                break;
            }
        }
        public void Rotate(float x, float y, float z, OPObject obj)
        {
            switch (obj)
            {
            case OPObject.Drawable:
                Test.Rotateobj(curObjIdx, x, y, z);
                break;
            case OPObject.Camera:
                if (x != 0.0f) //quick skip
                    Test.Camera.Pitch(x);
                if (y != 0.0f) //quick skip
                    Test.Camera.Yaw(y);
                if (z != 0.0f) //quick skip
                    Test.Camera.Roll(z);
                break;
            case OPObject.Light:
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
