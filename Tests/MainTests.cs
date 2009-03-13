using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using NUnit.Framework;
using System.Net.Mail;
using System.Net;
using System.Threading;

namespace DevSmtp.Tests
{
	[TestFixture]
	public class MainTests
	{
		[Test]
		public void SendMailTest()
		{
			SendEmail(
				"me@somehost.com",
				"someone@somehost.com",
				"Hi there!",
				"It's the body, no more.",
				true,
				IPAddress.Loopback.ToString()
			);
		}

		private void SendEmail(string from, string to, string subject, string body, bool isHtml, string host)
		{
			var message = new MailMessage(from, to);

			message.Body = body;
			message.Subject = subject;

			message.BodyEncoding = Encoding.UTF8;
			message.SubjectEncoding = Encoding.UTF8;

			var client = new SmtpClient(host);

			message.IsBodyHtml = isHtml;

			// wait a second. we should allow our server to start.
			Thread.Sleep(1000);

			client.SendAsync(message, null);
		}

		static void Main()
		{
			var tests = new MainTests();

			// Run one time
			tests.SendMailTest();

			// Run in a cycle
			for (int i = 0; i < 10; i++)
			{
				tests.SendMailTest();
			}
		}
	}
}
