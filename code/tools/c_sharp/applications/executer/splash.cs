using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Threading;
using System.Resources;
using System.Runtime.InteropServices;
using System.Drawing.Drawing2D;

namespace Executer
{

	/// <summary>
	/// Manage a crappy splash screen.
	/// </summary>
	public partial class Splash : Form
	{
/*
		#region boring

		private Splash()
		{
			InitializeComponent();
		}

		static Splash m_splash = null;

		/// <summary>
		/// Shows a splash screen. Creates it first if there is not already one present.
		/// </summary>
		/// <returns>true</returns>
		public static bool ShowSplash()
		{
			if(m_splash == null)
			{
				m_splash = new Splash();
				m_splash.Location = new Point(m_splash.Location.X, m_splash.Location.Y / 2);
			}

			m_splash.Show();
//			m_splash.label1.Update();
//			Application.Run(m_splash);

			return true;
		}

		/// <summary>
		/// Sets the visibility of the splash screen.
		/// </summary>
		/// <param name="bVisible">False if there is no splash screen to manage, otherwise true.</param>
		/// <returns></returns>
		public static bool SetVisible(bool bVisible)
		{
			if(m_splash != null)
			{
				m_splash.Visible = bVisible;
				return true;
			}
			return false;
		}

		/// <summary>
		/// Destroys the splash screen.
		/// </summary>
		/// <returns>False if there is no splash screen to manage, otherwise true.</returns>
		public static bool DestroySplash()
		{
			if(m_splash != null)
			{
				m_splash.Close();
				m_splash = null;
				return true;
			}
			return false;
		}

		/// <summary>
		/// Sets the progress bar in the splash screen and updates it.
		/// </summary>
		/// <param name="progress">Float progress value, 0..1</param>
		/// <returns>False if there is no splash screen to manage, otherwise true.</returns>
		public static bool SetProgress(float progress)
		{
			if(m_splash != null)
			{
				progress = progress > 1.0f ? 1.0f : progress;
				progress = progress < 0.0f ? 0.0f : progress;
				int newVal = (int)(progress * m_splash.progressBar1.Maximum);

				m_splash.progressBar1.Value = newVal;
				m_splash.progressBar1.Update();

				return true;
			}
			return false;
		}


		#endregion boring
*/
		#region fun
		/// <summary>
		/// Sets the visibility of the splash screen.
		/// </summary>
		/// <param name="bVisible">False if there is no splash screen to manage, otherwise true.</param>
		/// <returns></returns>
		public static bool SetVisible(bool bVisible)
		{
			if(s_splash != null)
			{
				s_splash.Visible = bVisible;
				return true;
			}
			return false;
		}

		public static bool ShowSplash()
		{
			if(s_splash == null)
			{
				s_splash = new Splash();
				s_splash.Location = new Point(s_splash.Location.X, s_splash.Location.Y / 2);
			}

			s_splash.FadeIn();
//			s_splash.Show();
			//			m_splash.label1.Update();
			//			Application.Run(m_splash);

			return true;
		}

		private Splash()
		{
			InitializeComponent();
//			Bitmap b = new Bitmap(this.BackgroundImage);
//			b.MakeTransparent(b.GetPixel(0,0));
//			this.BackgroundImage = b;

			m_image = panelMain.BackgroundImage;

//			panelMain.BackgroundImage = null;
			
			this.Width = m_image.Width;
			this.Height = m_image.Height;

			m_fadeValue = 0;
			m_fadeIncrement = 20;
			m_bAbort = false;

			m_state = State.Nothing;

//			this.Location = new Point(100, 100);
			this.Location = new Point((1280-Width)/2, (1024-512)/2); // HACK

			m_imageBackground = new Bitmap(this.Width, this.Height);
			Graphics g = Graphics.FromImage(m_imageBackground);
			g.CopyFromScreen(this.Location.X, this.Location.Y, 0, 0, this.Size);
			this.panelMain.BackgroundImage = m_imageBackground;
			g.Dispose();

			m_WaitForActivity = new AutoResetEvent(false);

			if(m_thread == null)
			{
				m_thread = new Thread(new ThreadStart(SplashThread));
				m_thread.Priority = ThreadPriority.AboveNormal;
				m_thread.Start();
			}

		}

		Thread m_thread;

		int m_fadeValue;
		int m_fadeIncrement;
		int m_fadeDecrement;
		bool m_bAbort;

		AutoResetEvent m_WaitForActivity;

		Image m_image = null;// = Executer.Properties.Resources.executer_large_banner;
		Image m_imageBackground = null;// = Executer.Properties.Resources.executer_large_banner;

//		static splash m_splash_screen = new splash();
		
		static Splash s_splash = null;

		/*public static Splash GetSplash()
		{
			if(s_splash == null)
			{
				s_splash = new Splash();
			}

			return s_splash;
		}
		*/
		public void FadeIn()
		{
			this.Show();
			ChangeState(State.FadeIn);
		}
		public void FadeOut()
		{
			this.Show();
			ChangeState(State.FadeOut);
		}

		enum State
		{
			Nothing = 0,
			Idle,
			FadeIn,
			FadeOut,
		}

		State m_state;

		bool ChangeState(State newState)
		{
			switch(newState)
			{
				case State.Nothing:
					{
						return ChangeState(State.Idle);
					}
//					break;

				case State.Idle:
					{
						m_state = newState;
					}
					break;

				case State.FadeIn:
					{
						m_fadeIncrement = 10;
						m_fadeValue = 0;

						m_state = newState;
//						this.Visible = false;
//
//						m_imageBackground = new Bitmap(this.Width, this.Height);
//						Graphics g = Graphics.FromImage(m_imageBackground);
//						g.CopyFromScreen(this.Location.X, this.Location.Y, 0, 0, this.Size);
//						this.panelMain.BackgroundImage = m_imageBackground;
//
//						this.Show();
//						this.Visible = true;
//						g.Dispose();


//						m_imageBackground = whatever is behind the window.
//						http://www.neowin.net/forum/lofiversion/index.php/t293883.html

					}
					break;

				case State.FadeOut:
					{
						m_fadeDecrement = 10;
//						m_fadeValue = m_image.Height;

						m_state = newState;
					}
					break;
			}
			m_WaitForActivity.Set();

			return true;
		}

		void KillThread()
		{
			this.m_thread.Abort();
		}
		
		void SplashThread()
		{
			int height = m_image.Height;
			int width = m_image.Width;

//			Graphics g = this.panelMain.CreateGraphics();
//			this.Show();

			Pen pen = System.Drawing.Pens.White;

			while(m_bAbort == false)
			{
				switch(m_state)
				{
					case State.Nothing:
						{
							m_WaitForActivity.WaitOne();
						}
						break;

					case State.Idle:
						{
							m_WaitForActivity.WaitOne();							
						}
						break;

					case State.FadeIn:
						{
							Rectangle srcRect = new Rectangle(0, m_fadeValue, width, m_fadeIncrement);
							Rectangle destRect = new Rectangle(this.Location.X, this.Location.Y+m_fadeValue, width, m_fadeIncrement);

							Rectangle oldDestRect = new Rectangle(this.Location.X, this.Location.Y + m_fadeValue - m_fadeIncrement, width, m_fadeIncrement);

							IntPtr deskDC = UnmanagedMethods.GetDC(IntPtr.Zero);

							Graphics gg = Graphics.FromHdc(deskDC);

							GraphicsContainer cont = gg.BeginContainer();
//							gg.FillRectangle(new SolidBrush(Color.FromArgb(100, Color.CadetBlue)), this.Location.X, this.Location.Y, width, m_fadeValue);
							gg.DrawImage(m_image, destRect, srcRect, GraphicsUnit.Pixel);
//							gg.DrawImage(m_imageBackground, oldDestRect, srcRect, GraphicsUnit.Pixel);

							gg.EndContainer(cont);

							gg.Dispose();





//this.Height = m_fadeValue;

							m_fadeValue += m_fadeIncrement;

							if(m_fadeValue > height)
							{
								m_fadeValue = height;
//								ChangeState(State.FadeOut);
								ChangeState(State.Idle);
							}

							Thread.Sleep(20);
						}
						break;

					case State.FadeOut:
						{
//							Rectangle rect = new Rectangle(0, m_fadeValue, width, m_fadeIncrement);
							Rectangle srcRect = new Rectangle(0, m_fadeValue, width, m_fadeDecrement);
							Rectangle destRect = new Rectangle(this.Location.X, this.Location.Y + m_fadeValue, width, m_fadeDecrement);

//							g.FillRectangle(System.Drawing.Brushes.White, rect);
//							g.DrawImage(m_imageBackground, rect, rect, GraphicsUnit.Pixel);




							IntPtr deskDC = UnmanagedMethods.GetDC(IntPtr.Zero);

							Graphics gg = Graphics.FromHdc(deskDC);

							GraphicsContainer cont = gg.BeginContainer();
							gg.DrawImage(m_imageBackground, destRect, srcRect, GraphicsUnit.Pixel);

							gg.EndContainer(cont);
							gg.Dispose();


							if(m_fadeValue == 0)
							{
//								ChangeState(State.Idle);
								this.Invoke(new DelegateMakeSplashInvisible(MakeSplashInvisible));
								m_bAbort = true;
							}

							m_fadeValue -= m_fadeDecrement;

							if(m_fadeValue < 0)
							{
								m_fadeValue = 0;
							}

							Thread.Sleep(20);
						}
						break;
				}


			}

//			g.Dispose();
		}

		delegate void DelegateMakeSplashInvisible();

		void MakeSplashInvisible()
		{
			this.Visible = false;
		}
		#region UnmanagedMethods
		internal class UnmanagedMethods
		{

			[DllImport("user32")]
			internal static extern IntPtr GetDC(IntPtr hwnd);

			[DllImport("User32.dll")]
			internal static extern void ReleaseDC(IntPtr dc);
		}
		#endregion

		#region DrawToScreen

		public class DrawToScreen
		{
			public void PaintRectangleToScreen()
			{
				IntPtr deskDC = UnmanagedMethods.GetDC(IntPtr.Zero);

				Graphics g = Graphics.FromHdc(deskDC);

				GraphicsContainer cont = g.BeginContainer();
				g.FillRectangle(new SolidBrush(Color.FromArgb(100, Color.CadetBlue)), 0, 0, 300, 300);

				g.EndContainer(cont);

				g.Dispose();

				UnmanagedMethods.ReleaseDC(deskDC);
			}
		}
		#endregion

		/// <summary>
		/// Destroys the splash screen.
		/// </summary>
		/// <returns>False if there is no splash screen to manage, otherwise true.</returns>
		public static bool DestroySplash()
		{
			if(s_splash != null)
			{
				s_splash.KillThread();
				s_splash.Close();
				s_splash = null;
				return true;
			}
			return false;
		}
		#endregion fun
	}
}
