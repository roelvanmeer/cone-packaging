/* $Id: cursesmessagedisplay.C,v 1.13 2009/06/27 17:12:00 mrsam Exp $
**
** Copyright 2003-2006, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "config.h"
#include "gettext.H"
#include "mymessage.H"
#include "myfolder.H"
#include "myserver.H"
#include "myservercallback.H"
#include "myserverpromptinfo.H"
#include "cursesmessage.H"
#include "cursesmessagedisplay.H"
#include "cursesattachmentdisplay.H"
#include "gettext.H"
#include "gpg.H"
#include "passwordlist.H"
#include "globalkeys.H"
#include "addressbook.H"
#include "htmlparser.H"
#include "disconnectcallbackstub.H"
#include "savedialog.H"
#include "curses/cursestitlebar.H"
#include "curses/cursescontainer.H"
#include "curses/cursesstatusbar.H"
#include "curses/cursesmainscreen.H"
#include "libmail/smtpinfo.H"
#include "maildir/maildirsearch.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <signal.h>
#include <errno.h>
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

using namespace std;

extern Gettext::Key key_LESSTHAN;
extern Gettext::Key key_ROT13;
extern Gettext::Key key_PRINT;
extern Gettext::Key key_TAKEADDR;
extern Gettext::Key key_FOLDERINDEX;
extern Gettext::Key key_DELETE;
extern Gettext::Key key_UNDELETE;
extern Gettext::Key key_NEXTMESSAGE;
extern Gettext::Key key_PREVMESSAGE;
extern Gettext::Key key_VIEWATT;
extern Gettext::Key key_SAVE;
extern Gettext::Key key_BOUNCE;
extern Gettext::Key key_REPLY;
extern Gettext::Key key_MSGSEARCH;
extern Gettext::Key key_FWD;
extern Gettext::Key key_HEADERS;
extern Gettext::Key key_UNENCRYPT;

extern CursesMainScreen *mainScreen;
extern CursesTitleBar *titleBar;
extern CursesStatusBar *statusBar;

extern void folderIndexScreen(void *);
extern void hierarchyScreen(void *);
extern void showAttachments(void *);

static string printCommand;

static void goNextMessage(void *vp)
{
	myFolder *f=(myFolder *)vp;
	size_t messageNum;

	f->getNextMessage(messageNum);
	f->goMessage(messageNum, "");
}

static void goNextUnreadMessage(void *vp)
{
	myFolder *f=(myFolder *)vp;
	size_t messageNum;

	f->getNextUnreadMessage(messageNum);
	f->goMessage(messageNum, "");
}

static void goPrevMessage(void *vp)
{
	myFolder *f=(myFolder *)vp;
	size_t messageNum;

	f->getPrevMessage(messageNum);
	f->goMessage(messageNum, "");
}

CursesMessageDisplay::CursesMessageDisplay(CursesContainer *parent,
				    CursesMessage *messageArg)
	: Curses(parent), CursesKeyHandler(PRI_SCREENHANDLER),
	  messageInfoPtr(messageArg)
{
	CursesMessage::currentDisplay=this;

	resized();
	folderUpdated();
}

void CursesMessageDisplay::folderUpdated()
{
	string inProgress="----";

	CursesMessage *messageInfo=messageInfoPtr;

	if (!messageInfo)
		return;

	if (!messageInfo->reformatting()) // Reformat finished
	{
		size_t nLines=messageInfo->nLines();
		size_t h=getHeight();

		size_t lastLine= nLines > h ? nLines-h:0;

		string buffer;

		{
			ostringstream o;

			o << (lastLine == 0 ? 100
			      : getFirstLineShown() * 100 / lastLine);
			buffer=o.str();
		}
		buffer += "%";

		inProgress=buffer;
	}

	titleBar->setTitles(Gettext(_("%1%: %2% of %3% %4%"))
			    << messageInfo->myfolder->getFolder()->getName()
			    << messageInfo->messagesortednum+1
			    << messageInfo->myfolder->getServer()
			    ->server->getFolderIndexSize()
			    << (messageInfo->orig_status_code == 'N'
				? _(" (NEW)"):
				messageInfo->orig_status_code == 'D'
				? _(" (DELETED)"): ""),
			    inProgress);
}

CursesMessageDisplay::~CursesMessageDisplay()
{
	CursesMessage::currentDisplay=NULL;
}

int CursesMessageDisplay::getWidth() const
{
	return getScreenWidth();
}

int CursesMessageDisplay::getHeight() const
{
	return (getParent()->getHeight());
}

void CursesMessageDisplay::draw()
{
	// About to redraw the screen, clear the link cache

	CursesMessage *messageInfo=messageInfoPtr;

	if (messageInfo)
		messageInfo->clearlinks();

	draw2();
}

void CursesMessageDisplay::draw2()
{
	int h=getHeight();

	int i;

	for (i=0; i<h; i++)
		drawLine(getFirstLineShown()+i);

}

void CursesMessageDisplay::drawLine(size_t lineNum)
{
	if (lineNum < getFirstLineShown())
		return;

	if (lineNum >= getFirstLineShown() + getHeight())
		return;

	size_t w=getWidth();

	CursesMessage *messageInfo=messageInfoPtr;

	if (!messageInfo)
	{
		Curses::CursesAttr attr;
		vector<wchar_t> l;

		l.insert(l.end(), w, ' ');
		l.push_back(0);
		writeText(&l[0], lineNum - getFirstLineShown(), 0, attr);
		return;
	}

	messageInfo->drawLine(lineNum, this, lineNum-getFirstLineShown());
}

void CursesMessageDisplay::resized()
{
	CursesMessage *messageInfo=messageInfoPtr;

	setFirstLineShown(0);

	if (messageInfo)
		messageInfo->beginReformat(getWidth());
	draw();
}

void CursesMessageDisplay::setFirstLineShown(size_t n)
{
	firstLineShown=n;
	firstLineToSearch=n;
}

bool CursesMessageDisplay::isFocusable()
{
	return true;
}

bool CursesMessageDisplay::processKeyInFocus(const Curses::Key &key)
{
	CursesMessage *messageInfo=messageInfoPtr;

	if (!messageInfo)
		return false;

	if (key == key.ENTER)
	{
		size_t row;
		size_t col;
		std::string url;

		if (!messageInfo->getCurrentLink(row, col, url))
			return true;


		if (url.substr(0, 7) == "mailto:")
		{
			if (myMessage::checkInterrupted(false) &&
			    (messageInfo=messageInfoPtr) != NULL)
				myMessage::newMessage(messageInfo->myfolder
						      ->getFolder(),
						      messageInfo->myfolder
						      ->getServer(),
						      url.substr(7));
			return true;
		}


		std::string handler;

		size_t p=url.find(':');

		if (p != url.npos)
		{
			handler=myServer::getConfigDir() + "/"
				+ url.substr(0, p) + ".handler";

			if (access(handler.c_str(), X_OK))
			{
				handler=FILTERDIR "/"
					+ url.substr(0, p) + ".handler";

				if (access(handler.c_str(), X_OK))
					handler="";
			}
		}

		if (handler.size() == 0)
		{
			statusBar->clearstatus();
			statusBar->
				status(Gettext(_("Cannot find handler for %1%"))
				       << url);
			statusBar->beepError();
			return true;
		}

		pid_t pp=fork();

		if (pp < 0)
		{
			statusBar->clearstatus();
			statusBar->status(strerror(errno));
			statusBar->beepError();
			return true;
		}

		if (pp == 0)
		{
			close(1);
			open("/dev/null", O_WRONLY);
			dup2(1, 2);
			dup2(1, 0);

			execl(handler.c_str(), handler.c_str(), url.c_str(),
			      (char *)NULL);

			exit(1);
		}

		pid_t p2;
		int waitstat;

		while ((p2=wait(&waitstat)) != pp)
		{
			if (p2 < 0 && errno != EINTR)
				break;
		}

		if (p2 == pp && WIFEXITED(waitstat) &&
		    WEXITSTATUS(waitstat) == 0)
		{
			statusBar->status(_("Started external handler."));
			return true;
		}

		statusBar->clearstatus();
		statusBar->status(_("External handler terminated with a non-zero exit code."),
				  statusBar->SYSERROR);
		return true;
	}

	if (key == key_TAKEADDR)
	{
		mail::envelope *e=messageInfo->getEnvelope();

		vector<mail::address> addrList;

		addrList.reserve(e->from.size() +
				 e->to.size() +
				 e->cc.size() +
				 e->bcc.size() +
				 e->sender.size() +
				 e->replyto.size());

		addrList.insert(addrList.end(),
				e->from.begin(),
				e->from.end());

		addrList.insert(addrList.end(),
				e->to.begin(),
				e->to.end());

		addrList.insert(addrList.end(),
				e->cc.begin(),
				e->cc.end());

		addrList.insert(addrList.end(),
				e->bcc.begin(),
				e->bcc.end());

		addrList.insert(addrList.end(),
				e->sender.begin(),
				e->sender.end());

		addrList.insert(addrList.end(),
				e->replyto.begin(),
				e->replyto.end());

		AddressBook::take(addrList);
		return true;
	}
	if (key == key_FOLDERINDEX ||
	    key == key_LESSTHAN)
	{
		keepgoing=false;
		myServer::nextScreen=&folderIndexScreen;
		myServer::nextScreenArg=messageInfo->myfolder;
		return true;
	}


	if (key == key_DELETE)
		messageInfo->myfolder->markDeleted( messageInfo->messagesortednum,
						    true, false);

	if (key == key_UNDELETE)
		messageInfo->myfolder->markDeleted( messageInfo->messagesortednum,
						    false, false);

	if (key == key_NEXTMESSAGE ||
	    key == key_DELETE ||
	    key == key_UNDELETE)
	{
		size_t dummy;

		if (messageInfo->myfolder->getNextMessage(dummy))
		{
			Curses::keepgoing=false;
			myServer::nextScreen= &goNextMessage;
			myServer::nextScreenArg=messageInfo->myfolder;
			return true;
		}
		return true;
	}

	if (key == key_PREVMESSAGE)
	{
		size_t dummy;

		if (messageInfo->myfolder->getPrevMessage(dummy))
		{
			Curses::keepgoing=false;
			myServer::nextScreen= &goPrevMessage;
			myServer::nextScreenArg=messageInfo->myfolder;
		}
		return true;
	}

	if (key == ' ' || key == Key::RIGHT ||
	    key == Key::PGDN || key == Key::DOWN)
	{
		if ((key == Key::DOWN || key == Key::RIGHT) &&
		    messageInfo->nextLink())
			return true; // Went to next link instead

		size_t nLines=messageInfo->nLines();
		size_t h=getHeight();

		size_t lastLine= nLines > h ? nLines-h:0;

		size_t firstLineSaved=getFirstLineShown();

		if (key == Key::DOWN || key == Key::RIGHT)
			setFirstLineShown(getFirstLineShown()+1);
		else
			setFirstLineShown(getFirstLineShown()+h);

		if (getFirstLineShown() > lastLine)
			setFirstLineShown(lastLine);

		if (firstLineSaved == getFirstLineShown() && key == ' ')
		{
			size_t dummy;

			if (messageInfo->myfolder->
			    getNextUnreadMessage(dummy))
			{
				Curses::keepgoing=false;
				myServer::nextScreen= &goNextUnreadMessage;
				myServer::nextScreenArg=messageInfo->myfolder;
				return true;
			}

			statusBar->clearstatus();
			statusBar->status(Gettext(_("No more unread mail in %1%"))
					  << messageInfo->myfolder
					  ->getFolder()->getName());
			return true;
		}

		folderUpdated();
		draw2();
		messageInfo->toLastLink();
		return true;
	}

	if (key == Key::LEFT || key == Key::PGUP || key == Key::UP)
	{
		if (key != Key::PGUP && messageInfo->prevLink())
			return true;

		size_t h=key != Key::PGUP ? 1:getHeight();

		setFirstLineShown(getFirstLineShown() < h ? 0:
				  getFirstLineShown()-h);
		folderUpdated();
		draw2();
		return true;
	}

	if (key == key_VIEWATT)
	{
		Curses::keepgoing=false;
		myServer::nextScreen=showAttachments;
		myServer::nextScreenArg=messageInfo;
		return true;
	}

	if (key == key_SAVE)
	{
		string filename;

		{
			SaveDialog save_dialog(filename);

			save_dialog.requestFocus();
			myServer::eventloop();

			filename=save_dialog;
			mainScreen->erase();
		}
		mainScreen->draw();
		requestFocus();

		if (messageInfoPtr.isDestroyed() || filename == "")
			return true;

		CursesAttachmentDisplay::downloadTo(messageInfoPtr,
						    messageInfoPtr->shownMimeId
						    != ""
						    ?
						    messageInfoPtr->structure
						    .find(messageInfoPtr->
							  shownMimeId)
						    :NULL,
						    filename);
		return true;
	}


	if (key == key_BOUNCE)
	{
		if (messageInfo->shownMimeId.size())
		{
			statusBar->clearstatus();
			statusBar->status(_("Cannot forward only an attachment, just the whole message."));
			statusBar->beepError();
			return true;
		}

		mail::smtpInfo sendInfo;

		string from;
		string replyto;
		string fcc;
		string customheaders;

		if (!myMessage::getDefaultHeaders(messageInfo->myfolder
						  ->getFolder(),
						  messageInfo->myfolder
						  ->getServer(),
						  from, replyto, fcc,
						  customheaders))
		{
			return true;
		}

		{
			vector<mail::address> addrList;
			size_t errIndex;

			if (mail::address::fromString(from, addrList,
						      errIndex)
			    && addrList.size() > 0)
				sendInfo.sender=addrList[0].getAddr();
		}

		mail::ptr<myFolder> folderPtr=messageInfo->myfolder;

		if (!CursesMessage::getBounceTo(sendInfo)
		    || !CursesMessage::getSendInfo(Gettext(_("Blind-forward message to %1% address(es)? (Y/N) "))
						   << sendInfo.recipients
						   .size(), "",
						   sendInfo, NULL))
			return true;

		disconnectCallbackStub disconnectStub;
		myServer::Callback sendCallback;

		mail::account *smtpServer;
		mail::folder *folder=
			CursesMessage::getSendFolder(sendInfo, smtpServer,
						     NULL,
						     disconnectStub);

		if (!folder)
			return true;

		if (folderPtr.isDestroyed() || messageInfoPtr.isDestroyed())
		{
			delete folder;
			if (smtpServer)
				delete smtpServer;
			return true;
		}

		try {
			messageInfo->copyContentsTo(folder, sendCallback);

			bool rc=myServer::eventloop(sendCallback);

			delete folder;
			folder=NULL;

			if (smtpServer)
			{
				if (rc)
				{
					myServer::Callback disconnectCallback;

					disconnectCallback.noreport=true;

					smtpServer->logout(disconnectCallback);
					myServer::
						eventloop(disconnectCallback);
				}

				delete smtpServer;
				smtpServer=NULL;
			}
			

		} catch (...) {

			if (folder)
				delete folder;
			if (smtpServer)
				delete smtpServer;
			LIBMAIL_THROW(LIBMAIL_THROW_EMPTY);
		}

		return true;
	}

	if (key == key_REPLY)
	{
		mail::ptr<myFolder> folder=messageInfo->myfolder;

		if (myMessage::checkInterrupted() &&
		    !folder.isDestroyed() &&
		    !messageInfoPtr.isDestroyed())
			messageInfo->reply();
		return true;
	}

	if (key == key_FWD)
	{
		mail::ptr<myFolder> folder=messageInfo->myfolder;

		if (myMessage::checkInterrupted() &&
		    !folder.isDestroyed() &&
		    !messageInfoPtr.isDestroyed())
			messageInfo->forward();
		return true;
	}

	if (key == key_HEADERS)
	{
		erase();
		messageInfo->fullEnvelopeHeaders=
			!messageInfo->fullEnvelopeHeaders;
		setFirstLineShown(0);
		messageInfo->beginReformat(getWidth());
		folderUpdated();
		return true;
	}

	if (key == key_PRINT)
	{
		mail::ptr<CursesMessage> ptr=messageInfo;
		int pipefd[2];
		pid_t pid1;

		myServer::promptInfo
			printPrompt(_("Print to: "));

		if (printCommand.size() == 0)
			printCommand="lpr";

		printPrompt=myServer::prompt(printPrompt
					     .initialValue(printCommand));

		if (printPrompt.abortflag || ptr.isDestroyed())
			return true;

		vector<const char *> args;

		{
			const char *p=getenv("SHELL");

			if (!p || !*p)
				p="/bin/sh";

			args.push_back(p);
		}

		args.push_back("-c");

		printCommand=printPrompt;

		args.push_back(printCommand.c_str());
		args.push_back(0);

		//
		// Fork a child process which writes the text image of the
		// message to a pipe.
		//
		// The parent process runs the print command, piping the text
		// into it.

		signal(SIGCHLD, SIG_DFL);

		if (pipe(pipefd) < 0)
		{
			statusBar->clearstatus();
			statusBar->status(strerror(errno));
			statusBar->beepError();
			return true;
		}

		if ((pid1=fork()) == 0)
		{
			FILE *fp;

			// First child exits, the second child generates the
			// text.

			if (fork())
				exit(0);
			signal(SIGPIPE, SIG_DFL);
			close(pipefd[0]);

			fp=fdopen(pipefd[1], "w");
			if (fp)
			{
				size_t nLines=messageInfo->nLines();
				size_t i;
				vector<pair<textAttributes, string> > line;

				vector<pair<textAttributes, string> >
					::iterator b, e;

				for (i=0; i<nLines; i++)
				{
					messageInfo->getLineImage(i, line);

					for (b=line.begin(), e=line.end();
					     b != e; ++b)
					{
						fprintf(fp, "%s",
							b->second.c_str());
					}
					fprintf(fp, "\n");
				}
				fflush(fp);
			}
			exit(0);
		}

		// Wait for the first child to exit.

		if (pid1 < 0)
		{
			close(pipefd[0]);
			close(pipefd[1]);
			statusBar->clearstatus();
			statusBar->status(strerror(errno));
			statusBar->beepError();
			return true;
		}

		close(pipefd[1]);

		while (wait(NULL) != pid1)
			;

		Curses::runCommand(args, pipefd[0], "");
		close(pipefd[0]);
		return true;
	}

	if (key == key_ROT13)
	{
		erase();
		messageInfo->rot13=!messageInfo->rot13;
		setFirstLineShown(0);
		messageInfo->beginReformat(getWidth());
		folderUpdated();
		return true;
	}

	if ((key == key_UNENCRYPT) &&
	    (messageInfo->isSigned() ||
	     messageInfo->isEncrypted()) &&
	    GPG::gpg.gpg_installed())
	{
		mail::ptr<CursesMessage> ptr=messageInfo;
		string passphrase;
		vector<string> options;
		bool wasEncrypted=messageInfo->isEncrypted();

		if (wasEncrypted)
		{
			if (!PasswordList::passwordList.check("decrypt:",
							      passphrase))
			{

				myServer::promptInfo
					passPrompt(_("Passphrase (if"
						     " required): "));

				passPrompt=myServer::prompt(passPrompt
							    .password());

				if (passPrompt.abortflag || ptr.isDestroyed())
					return true;

				passphrase=passPrompt;
			}
		}

		string::iterator b=GPG::gpg.extraDecryptVerifyOptions.begin();

		while (b != GPG::gpg.extraDecryptVerifyOptions.end())
		{
			if (isspace((int)(unsigned char)*b))
			{
				b++;
				continue;
			}

			string::iterator s=b;

			while (b != GPG::gpg.extraDecryptVerifyOptions.end())
			{
				if (isspace((int)(unsigned char)*b))
					break;
				b++;
			}

			options.push_back(string(s, b));
		}

		bool decryptFailed;

		if (messageInfo->decrypt(passphrase, options, decryptFailed))
		{
			if (wasEncrypted && !decryptFailed)
				PasswordList::passwordList
					.save("decrypt:", passphrase);

			erase();
			setFirstLineShown(0);
			messageInfo->beginReformat(getWidth());
			folderUpdated();

			if (wasEncrypted && decryptFailed)
			{
				statusBar->clearstatus();
				statusBar->status(_("ERROR: Decryption failed,"
						    " passphrase forgotten."));
				statusBar->beepError();
			}
		}

		return true;
	}
	
	if (key == key_MSGSEARCH)
	{
		mail::ptr<CursesMessage> ptr=messageInfo;

		myServer::promptInfo searchPrompt(_("Search: "));

		searchPrompt=myServer::prompt(searchPrompt
					      .initialValue(searchString));

		if (searchPrompt.abortflag || ptr.isDestroyed())
			return true;

		string search_utf8=Gettext::toutf8(searchPrompt);

		// Collapse multiple spaces in the search string into one
		// space.  Chop off leading and trailing spaces.

		string::iterator i, j, k;

		i=j=search_utf8.begin();
		k=search_utf8.end();

		while (i != k && *i == ' ')
			++i;

		while (i != k)
		{
			if (*i != ' ')
			{
				*j++ = *i++;
				continue;
			}

			while (i != k && *i == ' ')
				++i;

			if (i != k)
				*j++=' ';
		}

		search_utf8.erase(j, k);

		char *p= (*unicode_UTF8.tolower_func)(&unicode_UTF8,
						      search_utf8.c_str(),
						      NULL);

		if (!p)
		{
			statusBar->clearstatus();
			statusBar->status(strerror(errno));
			statusBar->beepError();
			return true;
		}

		try {
			search_utf8=p;
			free(p);
		} catch (...)
		{
			free(p);
			throw;
		}

		if (search_utf8.size() == 0)
			return true;

		searchString=searchPrompt;

		mail::Search searchEngine;

		if (searchEngine.setString(search_utf8) < 0)
		{
			statusBar->clearstatus();
			statusBar->status(strerror(errno));
			statusBar->beepError();
			return true;
		}
		searchEngine.reset();

		statusBar->clearstatus();
		statusBar->status(_("Searching..."));
		statusBar->flush();

		size_t n=messageInfo->nLines();

		vector<pair<textAttributes, string> > line;

		while (firstLineToSearch < n)
		{
			messageInfo->getLineImage(firstLineToSearch, line);

			string s;

			vector<pair<textAttributes, string> >
				::iterator b, e;

			for (b=line.begin(), e=line.end(); b != e; ++b)
				s += Gettext::toutf8(b->second);

			p= (*unicode_UTF8.tolower_func)(&unicode_UTF8,
							s.c_str(), NULL);

			if (!p)
			{
				statusBar->clearstatus();
				statusBar->status(strerror(errno));
				statusBar->beepError();
				return true;
			}

			try {
				s=p;
				free(p);
			} catch (...)
			{
				free(p);
				throw;
			}

			if (s.size() == 0)
			{
				++firstLineToSearch;
				continue;
			}

			i=s.begin();
			j=s.end();

			while (i != j)
			{
				if (*i != ' ')
				{
					searchEngine << *i;
					++i;
					continue;
				}

				while (i != j && *i == ' ')
					++i;
				if (i == j)
					break;
				searchEngine << ' ';
			}
			searchEngine << ' '; // EOL syntactically a space

			if (searchEngine)
			{
				size_t nLines=messageInfo->nLines();
				size_t h=getHeight();
				size_t lastLine= nLines > h ? nLines-h:0;

				size_t foundAt=firstLineToSearch;

				size_t n=foundAt > 0 ? foundAt-1:0;

				if (n > lastLine)
					n=lastLine;

				setFirstLineShown(n);
				firstLineToSearch=foundAt+1;

				statusBar->clearstatus();
				statusBar->status(Gettext(_("Text located,"
							    " displayed on"
							    " line #%1%"))
						  << (firstLineToSearch-n));
				draw();
				return true;
			}
			++firstLineToSearch;
		}
		statusBar->clearstatus();
		statusBar->status(_("Not found."));
		return true;
	}

	return false;
}

int CursesMessageDisplay::getCursorPosition(int &row, int &col)
{
	size_t lrow;
	size_t lcol;
	string url;

	CursesMessage *messageInfo=messageInfoPtr;

	if (messageInfo && messageInfo->getCurrentLink(lrow, lcol, url))
	{
		row=(int)lrow;
		col=(int)lcol;
		Curses::getCursorPosition(row, col);
		return 1;
	}

	Curses::getCursorPosition(row, col);
	return 0;
}

bool CursesMessageDisplay::processKey(const Curses::Key &key)
{
	CursesMessage *messageInfo=messageInfoPtr;

	if (!messageInfo)
		return false;

	return GlobalKeys::processKey(key, GlobalKeys::MESSAGESCREEN,
				      messageInfo->myfolder->getServer());
}

bool CursesMessageDisplay::listKeys( vector< pair<string, string> > &list)
{
	GlobalKeys::listKeys(list, GlobalKeys::MESSAGESCREEN);

	list.push_back(make_pair(Gettext::keyname(_("SAVE_K:S")),
				 _("Save")));

	list.push_back(make_pair(Gettext::keyname(_("TAKEADDR_K:T")),
				 _("Take Addr")));

	list.push_back(make_pair(Gettext::keyname(_("FOLDERINDEX_K:I")),
				 _("Index")));

	list.push_back(make_pair(Gettext::keyname(_("BOUNCE_K:B")),
				 _("Blind Fwd")));

	list.push_back(make_pair(Gettext::keyname(_("FWD_K:F")),
				 _("Fwd")));

	list.push_back(make_pair(Gettext::keyname(_("HEADERS_K:H")),
				 _("Full Hdrs")));

	list.push_back(make_pair(Gettext::keyname(_("NEXT_K:N")),
				 _("Next")));

	list.push_back(make_pair(Gettext::keyname(_("PREV_K:P")),
				 _("Prev")));

	list.push_back( make_pair(Gettext::keyname(_("DELETE_K:D")),
				 _("Delete")));

	list.push_back( make_pair(Gettext::keyname(_("UNDELETE_K:U")),
				 _("Undel/Unrd")));

	list.push_back(make_pair(Gettext::keyname(_("REPLY_K:R")),
				 _("Reply")));

	list.push_back(make_pair(Gettext::keyname(_("ROT13_K:^R")),
				 _("Rot 13")));

	list.push_back(make_pair(Gettext::keyname(_("PRINT_K:|")),
				 _("Print")));

	list.push_back(make_pair(Gettext::keyname(_("VIEWATT_K:V")),
				 _("View Attchs")));

	list.push_back(make_pair(Gettext::keyname(_("SPACE_K:SP")),
				 _("Next Unread")));

	list.push_back(make_pair(Gettext::keyname(_("SEARCH_K:/")),
				 _("Search")));

	if (!messageInfoPtr.isDestroyed() &&
	    (messageInfoPtr->isSigned() ||
	     messageInfoPtr->isEncrypted()) &&
	    GPG::gpg.gpg_installed())
	{
		list.push_back(make_pair(Gettext::keyname(_("UNENCRYPT_K:Y")),
					 _("decrYpt/Vfy")));
	}
	return false;
}


