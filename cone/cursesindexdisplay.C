/* $Id: cursesindexdisplay.C,v 1.23 2009/06/27 17:12:00 mrsam Exp $
**
** Copyright 2003-2006, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "config.h"
#include "cursesindexdisplay.H"
#include "messagesize.H"
#include "mymessage.H"
#include "cursesmessage.H"
#include "specialfolder.H"
#include "myserver.H"
#include "myservercallback.H"
#include "myserverpromptinfo.H"
#include "myreferences.H"
#include "searchcallback.H"
#include "tags.H"
#include "disconnectcallbackstub.H"
#include "curses/cursescontainer.H"
#include "curses/cursestitlebar.H"
#include "libmail/mail.H"
#include "libmail/search.H"
#include "libmail/smtpinfo.H"
#include "rfc822/rfc822.h"
#include "rfc822/rfc2047.h"
#include "unicode/unicode.h"
#include "gettext.H"
#include "globalkeys.H"
#include "searchprompt.H"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <errno.h>
#include <time.h>

using namespace std;

extern Gettext::Key key_CLEAR;
extern Gettext::Key key_SEARCHBROADEN;
extern Gettext::Key key_SEARCHNARROW;
extern Gettext::Key key_SORTFOLDER_ARRIVAL;
extern Gettext::Key key_SORTFOLDER_DATE;
extern Gettext::Key key_SORTFOLDER_NAME;
extern Gettext::Key key_SORTFOLDER_SUBJECT;
extern Gettext::Key key_SORTFOLDER_THREAD;
extern Gettext::Key key_SORTNOT;
extern Gettext::Key key_TOGGLE;
extern Gettext::Key key_BLOBS;
extern Gettext::Key key_WATCHDAYS;
extern Gettext::Key key_WATCHDEPTH;
extern Gettext::Key key_MARKUNMARK;
extern Gettext::Key key_SEARCH;
extern Gettext::Key key_SORTFOLDER;
extern Gettext::Key key_NEXTMESSAGE;
extern Gettext::Key key_PREVMESSAGE;

extern Gettext::Key key_TAG0;
extern Gettext::Key key_TAG1;
extern Gettext::Key key_TAG2;
extern Gettext::Key key_TAG3;
extern Gettext::Key key_TAG4;
extern Gettext::Key key_TAG5;
extern Gettext::Key key_TAG6;
extern Gettext::Key key_TAG7;
extern Gettext::Key key_TAG8;
extern Gettext::Key key_TAG9;

extern char udelete[];
extern char ucheck[];
extern char unew[];
extern wchar_t uhoriz;
extern wchar_t uvert;
extern wchar_t uupright;
extern wchar_t urighttee;
extern wchar_t uwatch, uwatchend;
extern bool usesampm;

extern CursesTitleBar *titleBar;
extern CursesStatusBar *statusBar;
extern void hierarchyScreen(void *);
extern void showCurrentMessage(void *);
extern void folderIndexScreen(void *);
extern void hierarchyScreen(void *);

extern Gettext::Key key_BOUNCE;
extern Gettext::Key key_REPLY;
extern Gettext::Key key_FWD;
extern Gettext::Key key_DELETE;
extern Gettext::Key key_WATCH;
extern Gettext::Key key_UNDELETE;
extern Gettext::Key key_UNREADBULK;
extern Gettext::Key key_EXPUNGE;
extern Gettext::Key key_TAG;
extern Gettext::Key key_COPY;
extern Gettext::Key key_JUMP;
extern Gettext::Key key_VIEWATT;
extern Gettext::Key key_MOVEMESSAGE;

//////////////////////////////////////////////////////////////////////////
//
// Helper class that's used to check for new mail

class CursesIndexDisplay::CheckNewMail : public mail::callback {
public:
	CheckNewMail();
	~CheckNewMail();

	void success(string);
	void fail(string);
};

CursesIndexDisplay::CursesIndexDisplay(CursesContainer *parentArg,
				       myFolder *folderArg)
	: Curses(parentArg), myFolder::IndexDisplay(folderArg),
	  CursesKeyHandler(PRI_SCREENHANDLER), action(no_action)
{
	folderArg->screenOpened();
}

CursesIndexDisplay::~CursesIndexDisplay()
{
}

	// Inherited from Curses:

int CursesIndexDisplay::getWidth() const
{
	return getScreenWidth();
}

int CursesIndexDisplay::getHeight() const
{
	myFolder *f=getFolderIndex();

	return f ? f->size():0;
}

void CursesIndexDisplay::draw()
{
	myFolder *f=getFolderIndex();

	if (f)
		titleBar->setTitles(f->getFolder()->getName(),
				    Gettext(_("%1% messages"))
				    << f->size());

	size_t h=getScreenHeight();

	size_t cursorPos=f ? f->getCurrentMessage():0;

	size_t n=0;

	if (cursorPos > h)
		n=cursorPos-h; // Make sure the screen gets drawn

	while (n < cursorPos + h)
		drawLine(n++);
}

void CursesIndexDisplay::draw(size_t n)
{
	drawLine(n);
}

// Draw a line of an index.

void CursesIndexDisplay::drawLine(size_t row)
{
	char buffer[100];
	char buffer2[100];

	// Compute the size of the largest message number.

	size_t messageNumLength;

	{
		string s;

		{
			ostringstream o;

			o << getHeight();
			s=o.str();
		}

		messageNumLength=s.size();
	}


	size_t w=getWidth();
	vector<wchar_t> line;

	line.insert(line.begin(), w, ' ');

	if (row >= (size_t)getHeight()) // Trailing blank lines
	{
		line.push_back(0);
		writeText(&*line.begin(), row, 0, Curses::CursesAttr());
		return;
	}

	vector<wchar_t>::iterator lineb=line.begin(), linee=line.end();

	myFolder *f=getFolderIndex();

	const myFolder::Index &i=f->getIndex(row);

	if (lineb != linee)
	{
		vector<wchar_t> status;

		Curses::mbtow(i.status_code == 'D' ? udelete:
			      i.status_code == 'R' ? _("R"):
			      i.status_code == 'N' ? unew:_(" "), status);

		*lineb++ = status.size() > 0 ? status[0]:' ';
	}

	// F nnn

	bool marked=f->getFlags(row).marked;

	string msgNumBuf;

	if (lineb != linee)
	{
		vector<wchar_t> status;

		Curses::mbtow(ucheck, status);

		*lineb++ = marked && status.size() > 0 ? status[0]:' ';

		{
			ostringstream s;

			s << setw(messageNumLength) << (row+1);

			msgNumBuf=s.str();

		}
	}

	{
		vector<wchar_t> messageNumWC;

		mbtow(msgNumBuf.c_str(), messageNumWC);

		vector<wchar_t>::iterator b=messageNumWC.begin(),
			e=messageNumWC.end();

		while (b != e && lineb != linee)
			*lineb++ = *b++;

		if (lineb != linee)
			lineb++;
	}

	// Dates less than a week old show ddd hh:mm
	// Other dates show just the date portion
	// Compute both formats, and use the largest string for metrics

	time_t now, messageDate;
	time (&now);

	messageDate=now;

	if (i.messageDate)
		messageDate=i.messageDate;
	else if (i.arrivalDate)
		messageDate=i.arrivalDate;

	strftime(buffer, sizeof(buffer),
		 usesampm ? "%a %I:%M %p":"%a %H:%M",
		 localtime(&messageDate));
	strftime(buffer2, sizeof(buffer2), "%d-%b-%Y",
		 localtime(&messageDate));

	{
		vector<wchar_t> fmt1, fmt2;

		mbtow(buffer, fmt1);
		mbtow(buffer2, fmt2);

		if (fmt1.size() < fmt2.size())
			fmt1.insert(fmt1.begin(), fmt2.size() - fmt1.size(),
				    ' ');
		else
			fmt2.insert(fmt2.begin(), fmt1.size() - fmt2.size(),
				    ' ');

		vector<wchar_t> &f=
			messageDate <= now &&
			messageDate + 60 * 60 * 24 * 6 >= now ? fmt1:fmt2;

		vector<wchar_t>::iterator b=f.begin(), e=f.end();

		while (b != e && lineb != linee)
			*lineb++ = *b++;

		if (lineb != linee)
			lineb++;
	}

	wchar_t watchChar[2];

	watchChar[0]=i.watchLevel == 0 ? 0:
		i.watchLevel == 1 ? uwatchend:uwatch;
	watchChar[1]=0;
	size_t watchPos=0;

	// What we've got left, half of it goes to the subject/size,
	// the other half to the sender.

	size_t sw= (linee - lineb) / 2;

	if (sw > 0)
		--sw;

	{
		vector<wchar_t> name;

		char *name_locale=
			unicode_cfromutf8(Gettext::defaultCharset(),
					  i.name_utf8.c_str(), NULL);

		if (name_locale)
			try {
				mbtow(name_locale, name);
				free(name_locale);
			} catch (...) {
				free(name_locale);
				LIBMAIL_THROW(LIBMAIL_THROW_EMPTY);
			}

		// Pad subject to field width

		if (name.size() > sw)
			name.erase(name.begin() + sw, name.end());
		else
			name.insert(name.end(), sw - name.size(), ' ');


		// Right align message size

		string sizeStr=MessageSize(i.messageSize);

		buffer[0]=0;

		if (sizeStr.size() > 0)
			sprintf(buffer, " (%s)", sizeStr.c_str());

		vector<wchar_t>::iterator b, e;
		b=name.begin();
		e=name.end();

		vector<wchar_t> msgLength;
		mbtow(buffer, msgLength);

		vector<wchar_t>::iterator bb=msgLength.begin(),
			ee=msgLength.end();

		while (b != e && bb != ee)
			*--e = *--ee;

		if (b != e)
			*--e = ' ';

		b=name.begin();
		e=name.end();

		while (b != e && lineb != linee)
			*lineb++ = *b++;
	}


	if (lineb != linee)
	{
		watchPos=lineb - line.begin();
		++lineb;
	}


	sw=linee - lineb;

	{
		string subjectStr=i.subject_utf8;

		if (getFolderIndex()->getServer()
		    ->getFolderConfiguration(getFolderIndex()->getFolder(),
					     "INDEXBLOBS") == "1")
		{
			int subj_flags;
			char *p=rfc822_coresubj_nouc(subjectStr.c_str(),
						     &subj_flags);

			if (!p)
				LIBMAIL_THROW (strerror(errno));

			try {
				subjectStr=p;
				free(p);
			} catch (...)
			{
				free(p);
				LIBMAIL_THROW(LIBMAIL_THROW_EMPTY);
			}

			if (subj_flags & CORESUBJ_RE)
				subjectStr="Re: " + subjectStr;

			if (subj_flags & CORESUBJ_FWD)
				subjectStr=subjectStr + " (fwd)";
		}

		vector<wchar_t> subject;

		char *subject_locale=
			unicode_cfromutf8(Gettext::defaultCharset(),
					  subjectStr.c_str(), NULL);

		if (subject_locale)
			try {
				mbtow(subject_locale, subject);
				free(subject_locale);
			} catch (...) {
				free(subject_locale);
				LIBMAIL_THROW(LIBMAIL_THROW_EMPTY);
			}

		vector<wchar_t> thread_pad;

		thread_pad.insert(thread_pad.end(), i.threadLevel * 2, ' ');

		if (i.threadLevel > 0)
		{
			size_t p = i.threadLevel * 2 -2;

			vector<size_t>::const_iterator
				b=i.active_threads.begin(),
				e=i.active_threads.end();

			while (b != e)
			{
				size_t n= *b++ * 2;

				if (n < thread_pad.size())
					thread_pad[n]=uvert;
			}

			thread_pad[p]=uupright;

			if (row + 1 < f->size() && i.active_threads.size() > 0)
			{
				size_t j=i.active_threads.size()-1;

				const myFolder::Index &next=f->getIndex(row+1);

				if (next.active_threads.size()
				    >= i.active_threads.size()
				    && next.active_threads[j]
				    == i.active_threads[j])
					thread_pad[p]=urighttee;
			}
			thread_pad[p+1]=uhoriz;

			subject.insert(subject.begin(), thread_pad.begin(),
				       thread_pad.end());
		}

		if (subject.size() > sw)
			subject.erase(subject.begin() + sw, subject.end());
		else
			subject.insert(subject.end(), sw - subject.size(), ' ');

		vector<wchar_t>::iterator b, e;

		b=subject.begin();
		e=subject.end();

		while (b != e && lineb != linee)
			*lineb++ = *b++;
	
	}
	
	line.push_back(0);

	Curses::CursesAttr attr;

	attr.setHighlight(marked);
	attr.setBgColor(i.tag);

	bool isReverse=row == f->getCurrentMessage();

	attr.setReverse(isReverse);

	writeText(&line[0], row, 0, attr);

	if (watchChar[0])
	{
		attr.setReverse(!isReverse);
		writeText(watchChar, row, watchPos, attr);
	}
}

bool CursesIndexDisplay::isFocusable()
{
	return 1;
}

bool CursesIndexDisplay::processKeyInFocus(const Curses::Key &key)
{
	myFolder *f=getFolderIndex();

	if (!f)
		return false;

	if (key == key.DOWN || key == key_NEXTMESSAGE)
	{
		size_t oldLine=f->getCurrentMessage();
		size_t newLine=oldLine;

		if (newLine + 1 >= f->size())
		{
			myServer::Callback checkNewMailCallback;

			f->checkNewMail(checkNewMailCallback);

			myServer::eventloop(checkNewMailCallback);
			return true;
		}
		++newLine;

		f->setCurrentMessage(newLine);
		drawLine(oldLine);
		drawLine(newLine);
		return true;
	}


	if (f->getCurrentMessage() >= f->size())
		return false;

	if (key == key.ENTER)
	{
		SpecialFolder &d=SpecialFolder::folders.find(DRAFTS)
			->second;

		string draftServer;
		string draftPath;

		if (f->getFlags(f->getCurrentMessage()).draft ||
		    (
		     d.getSingleFolder(draftServer, draftPath) &&
		     f->getServer()->url == draftServer &&
		     f->getFolder()->toString() == draftPath)
		    )
		{
			mail::ptr<mail::account> serverPtr=f->getServer()->server;

			if (myMessage::checkInterrupted() &&
			    !serverPtr.isDestroyed())
				f->goDraft();
			return true;
		}

		f->goMessage(f->getCurrentMessage(), "");
		return true;
	}

	if (key == key_BOUNCE)
	{
		mail::smtpInfo sendInfo;

		string from;
		string replyto;
		string fcc;
		string customheaders;

		if (!myMessage::getDefaultHeaders(f->getFolder(),
						  f->getServer(),
						  from, replyto, fcc,
						  customheaders))
			return true;


		{
			vector<mail::address> addrList;
			size_t errIndex;

			if (mail::address::fromString(from, addrList,
						      errIndex)
			    && addrList.size() > 0)
				sendInfo.sender=addrList[0].getAddr();
		}

		mail::ptr<mail::account> serverPtr=f->getServer()->server;
		mail::ptr<myFolder> folderPtr=f;

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

		size_t n;

		if (serverPtr.isDestroyed() || folderPtr.isDestroyed()
		    || (n=folderPtr->getCurrentMessage()) >= folderPtr->size())
		{
			delete folder;
			if (smtpServer)
				delete smtpServer;
			return true;
		}

		n=folderPtr->getServerIndex(n);

		try {
			vector<size_t> msgList;

			msgList.push_back(n);

			serverPtr->copyMessagesTo(msgList,
						  folder, sendCallback);

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
		mail::ptr<mail::account> serverPtr=f->getServer()->server;

		if (myMessage::checkInterrupted() &&
		    !serverPtr.isDestroyed())
			f->goMessage(f->getCurrentMessage(), "",
				     &myMessage::reply);
		return true;
	}

	if (key == key_FWD)
	{
		mail::ptr<mail::account> serverPtr=f->getServer()->server;

		if (myMessage::checkInterrupted() &&
		    !serverPtr.isDestroyed())
			f->goMessage(f->getCurrentMessage(), "",
				     &myMessage::forward);
		return true;
	}

	if (key == key_DELETE)
	{
		size_t n=f->getCurrentMessage();

		f->markDeleted(n, true, true);

		if (n + 1 < f->size())
		{
			++n;
			f->setCurrentMessage(n);
			drawLine(n-1);
			drawLine(n);
		}
		return true;
	}

	if (key == key_WATCH)
	{
		size_t n=f->getCurrentMessage();

		if (n >= f->size())
			return true;

		mail::ptr<mail::account> serverPtr=f->getServer()->server;

		if (f->getIndex(n).watchLevel <= 1)
		{
			unsigned nDays;
			unsigned nLevels;

			if (!watchPrompt(nDays, nLevels) ||
			    serverPtr.isDestroyed())
				return true;

			f->watch(n, nDays, nLevels+1);
			return true;
		}

		if (f->getIndex(n).watchLevel > 1)
		{
			myServer::promptInfo prompt=
				myServer::promptInfo(_("Stop watching this "
						       "thread? (Y/N) "))
				.yesno();

			prompt=myServer::prompt(prompt);

			if (serverPtr.isDestroyed() || prompt.abortflag ||
			    (string)prompt != "Y")
				return true;

			f->unwatch(n);
		}
		return true;
	}

	if (key == key_UNDELETE)
	{
		size_t n=f->getCurrentMessage();

		f->markDeleted(n, false, true);

		if (n + 1 < f->size())
		{
			++n;
			f->setCurrentMessage(n);
			drawLine(n-1);
			drawLine(n);
		}
		return true;
	}

	if (key == key_MARKUNMARK)
	{
		size_t n=f->getCurrentMessage();

		f->toggleMark(n);

		if (n + 1 < f->size())
		{
			++n;
			f->setCurrentMessage(n);
			drawLine(n-1);
			drawLine(n);
		}
		return true;
	}

	if (key == key_EXPUNGE)
	{
		myServer *s=f->getServer();

		if (f->mymessage)
		{
			delete f->mymessage;
			f->mymessage=NULL;
		}

		if (s && s->server)
		{
			myServer::Callback callback;

			s->server->updateFolderIndexInfo(callback);

			myServer::eventloop(callback);
		}
		return true;
	}

	if (key == key_TAG)
	{
		size_t tagNum;

		if (!getTag(_("Tag: "), tagNum))
			return true;

		f->setTag(f->getCurrentMessage(), tagNum);
		return true;
	}

	if (key == key.LEFT)
	{
		size_t oldLine=f->getCurrentMessage();
		size_t newLine=oldLine;

		while (newLine > 0)
		{
			if (f->getFlags(--newLine).marked)
			{
				f->setCurrentMessage(newLine);
				drawLine(oldLine);
				drawLine(newLine);
				return true;
			}
		}
		return true;
	}

	if (key == key.UP || key == key_PREVMESSAGE)
	{
		size_t oldLine=f->getCurrentMessage();
		size_t newLine=oldLine;

		if (newLine > 0)
			--newLine;

		f->setCurrentMessage(newLine);
		drawLine(oldLine);
		drawLine(newLine);
		return true;
	}

	if (key == key.PGUP)
	{
		size_t oldLine=f->getCurrentMessage();
		size_t newLine=oldLine;
		size_t firstRow;
		size_t h;

		getVerticalViewport(firstRow, h);
		if (h < 1)
			h=1;

		if (oldLine > firstRow)
		{
			newLine=firstRow;
		}
		else
		{
			if (newLine > h)
				newLine -= h;
			else newLine=0;
		}

		f->setCurrentMessage(newLine);
		drawLine(oldLine);
		drawLine(newLine);
		return true;
	}

	if (key == key.RIGHT)
	{
		size_t oldLine=f->getCurrentMessage();
		size_t newLine=oldLine;

		do
		{
			if (++newLine >= f->size())
				return true;
		} while (!f->getFlags(newLine).marked);

		f->setCurrentMessage(newLine);
		drawLine(oldLine);
		drawLine(newLine);
		return true;
	}

	if (key == key.PGDN)
	{
		size_t oldLine=f->getCurrentMessage();
		size_t newLine=oldLine;
		size_t firstRow;
		size_t h;

		getVerticalViewport(firstRow, h);

		if (h < 1)
			h=1;

		if (firstRow + h != oldLine + 1)
		{
			newLine=firstRow+h;

			if (newLine > f->size())
				newLine=f->size();

			if (newLine > 0)
				--newLine;
		}
		else
		{
			newLine += h;

			if (newLine >= f->size())
			{
				newLine=f->size();
				if (newLine > 0)
					--newLine;
			}
		}
		f->setCurrentMessage(newLine);
		drawLine(oldLine);
		drawLine(newLine);
		return true;
	}

	if (key == key.HOME)
	{
		size_t oldLine=f->getCurrentMessage();

		f->setCurrentMessage(0);
		drawLine(oldLine);
		drawLine(0);
		return true;
	}

	if (key == key.END)
	{
		size_t oldLine=f->getCurrentMessage();
		size_t newLine=f->size();

		if (newLine > 0)
			--newLine;

		f->setCurrentMessage(newLine);
		drawLine(oldLine);
		drawLine(newLine);
		return true;
	}

	if (key == key_COPY)
	{
		keepgoing=false;
		action=copy_single;
		return true;
	}

	if (key == key_MOVEMESSAGE)
	{
		keepgoing=false;
		action=move_single;
		return true;
	}

	if (key == key_SEARCH)
	{
		mail::ptr<myFolder> folder=f;

		mail::account *p=f->getServer()->server;

		if (!p)
			return true;

		size_t n=p->getFolderIndexSize();

		size_t i;
		for (i=0; i<n; i++)
			if (p->getFolderIndexInfo(i).marked)
				break;

		bool broaden=true;

		if (i < n) // Something already marked
		{
			if (searchPromptBroadenNarrow(folder, broaden))
				return true;
		}

		mail::searchParams searchInfo;

		bool selectAll;
		bool rc=searchPrompt::prompt(searchInfo, selectAll);

		if (folder.isDestroyed() ||
		    folder->getServer()->server == NULL ||
		    myServer::nextScreen)
		{
			Curses::keepgoing=false;
			if (!myServer::nextScreen)
			{
				myServer::nextScreen=&hierarchyScreen;
				myServer::nextScreenArg=NULL;
			}
			return true;
		}

		if (selectAll)
		{
			vector<size_t> msgNums;

			size_t n=p->getFolderIndexSize();

			size_t i;

			for (i=0; i<n; i++)
				msgNums.push_back(i);

			myServer::Callback callback;

			mail::messageInfo flags;

			flags.marked=true;

			p->updateFolderIndexFlags(msgNums,
						  false, true, flags,
						  callback);

			myServer::eventloop(callback);
			return true;
		}

		if (!rc)
			return true;

		searchInfo.scope=broaden ? searchInfo.search_unmarked:
			searchInfo.search_marked;

		if (!broaden)
			searchInfo.searchNot= !searchInfo.searchNot;

		statusBar->clearstatus();
		statusBar->status(_("Searching..."));

		myServer::Callback callback;

		SearchCallbackHandler searchCallback(callback);

		folder->getServer()->server
			->searchMessages(searchInfo,
					 searchCallback);

		if (!myServer::eventloop(callback)
		    || folder.isDestroyed()
		    || folder->getServer()->server == NULL)
			return true;

		myServer::Callback callback2;

		mail::messageInfo flags;

		flags.marked=true;

		folder->getServer()->server
			->updateFolderIndexFlags(searchCallback.messageNumbers,
						 false, broaden, flags,
						 callback2);

		myServer::eventloop(callback2);
		return true;
	}

	if (key == key_JUMP)
	{
		mail::ptr<myFolder> folder=f;
		string s;
		size_t n;

		myServer::promptInfo prompt=
			myServer
			::prompt(myServer
				 ::promptInfo(_("Jump to message #: ")));

		if (prompt.abortflag || folder.isDestroyed() ||
		    (s=prompt).size() == 0)
			return true;

		n=atol(s.c_str());

		if (n <= 0 || n > f->size())
			return true;

		size_t o=f->getCurrentMessage();

		--n;
		f->setCurrentMessage(n);
		drawLine(o);
		drawLine(n);
		return true;
	}

	return false;
}

bool CursesIndexDisplay::getTag(string promptStr, size_t &tagNum)
{
	myServer::promptInfo info=
		myServer::promptInfo(promptStr)
		.option(key_TAG0, Gettext::keyname(_("KEY_0:0")),
			Tags::tags.names[0])
		.option(key_TAG1, Gettext::keyname(_("KEY_1:1")),
			"/1" + Tags::tags.names[1])
		.option(key_TAG2, Gettext::keyname(_("KEY_2:2")),
			"/2" + Tags::tags.names[2])
		.option(key_TAG3, Gettext::keyname(_("KEY_3:3")),
			"/3" + Tags::tags.names[3])
		.option(key_TAG4, Gettext::keyname(_("KEY_4:4")),
			"/4" + Tags::tags.names[4])
		.option(key_TAG5, Gettext::keyname(_("KEY_5:5")),
			"/5" + Tags::tags.names[5])
		.option(key_TAG6, Gettext::keyname(_("KEY_6:6")),
			"/6" + Tags::tags.names[6])
		.option(key_TAG7, Gettext::keyname(_("KEY_7:7")),
			"/7" + Tags::tags.names[7])
		.option(key_TAG8, Gettext::keyname(_("KEY_8:8")),
			"/8" + Tags::tags.names[8])
		.option(key_TAG9, Gettext::keyname(_("KEY_9:9")),
			"/9" + Tags::tags.names[9]);


	info=myServer::prompt(info);

	if (info.abortflag || myServer::nextScreen)
		return false;

	vector<wchar_t> ka;

	Curses::mbtow( ((string)info).c_str(), ka);
	if (ka.size() == 0)
		return false;

	if (ka[0] < '0' || ka[0] > '9')
		return false;

	tagNum=ka[0]-'0';
	return true;
}

// Prompt to broaden or narrow the search

bool CursesIndexDisplay::searchPromptBroadenNarrow(mail::ptr<myFolder> &folder,
						   bool &broaden)
{
	myServer::promptInfo prompt=
		myServer::prompt(myServer::promptInfo(_("Modify: "))

				 .option(key_SEARCHBROADEN,
					 Gettext::keyname(_("BROADEN_K:B")),
							 _("Broaden search"))
				 .option(key_SEARCHNARROW,
					 Gettext::keyname(_("NARROW_K:N")),
					 _("Narrow search"))

				 .option(key_CLEAR,
					 Gettext::keyname(_("CLEAR_K:R")),
					 _("cleaR tagged"))

				 .option(key_COPY,
					 Gettext::keyname(_("COPY_K:C")),
					 _("Copy"))

				 .option(key_MOVEMESSAGE,
					 Gettext::keyname(_("MOVE_K:O")),
					 _("mOve"))

				 .option(key_TOGGLE,
					 Gettext::keyname(_("TOGGLE_K:G")),
					 _("toGgle"))

				 .option(key_DELETE,
					 Gettext::keyname(_("DELETE_K:D")),
					 _("Delete selected"))

				 .option(key_TAG,
					 Gettext::keyname(_("TAG_K:T")),
					 _("Tag selected"))

				 .option(key_UNDELETE,
					 Gettext::keyname(_("UNDELETE_K:U")),
					 _("Undelete selected"))

				 .option(key_UNREADBULK,
					 Gettext::keyname(_("UNREAD_K:E")),
					 _("unrEad selected"))

				 );

	mail::account *p;

	if (prompt.abortflag || folder.isDestroyed() ||
	    (p=folder->getServer()->server) == NULL)
		return true;

	vector<wchar_t> ka;

	Curses::mbtow( ((string)prompt).c_str(), ka);
	if (ka.size() == 0)
		return true;

	if (key_SEARCHNARROW == ka[0])
		broaden=false;

	if (key_DELETE == ka[0] ||
	    key_UNDELETE == ka[0] ||
	    key_UNREADBULK == ka[0])
	{
		vector<size_t> msgNums;

		size_t n=p->getFolderIndexSize();

		size_t i;

		for (i=0; i<n; i++)
		{
			mail::messageInfo m=p->getFolderIndexInfo(i);

			if (m.marked)
				msgNums.push_back(i);
		}

		myServer::Callback callback;

		mail::messageInfo flags;

		if (key_UNREADBULK == ka[0])
			flags.unread=true;
		else
			flags.deleted=true;

		p->updateFolderIndexFlags(msgNums, false,
					  key_DELETE == ka[0] ||
					  key_UNREADBULK == ka[0], flags,
					  callback);
		myServer::eventloop(callback);
		return true; // We're done.
	}

	if (key_CLEAR == ka[0])
	{
		vector<size_t> msgNums;

		size_t n=p->getFolderIndexSize();

		size_t i;

		for (i=0; i<n; i++)
			msgNums.push_back(i);

		myServer::Callback callback;

		mail::messageInfo flags;

		flags.marked=true;

		p->updateFolderIndexFlags(msgNums, false, false, flags,
					  callback);

		myServer::eventloop(callback);
		return true; // We're done.
	}

	if (key_COPY == ka[0])
	{
		keepgoing=false;
		action=copy_batch;
		return true;
	}

	if (key_MOVEMESSAGE == ka[0])
	{
		keepgoing=false;
		action=move_batch;
		return true;
	}

	if (key_TAG == ka[0])
	{
		size_t tagNum;
		size_t n;
		size_t i;
		size_t cnt=0;

		n=p->getFolderIndexSize();

		for (i=0; i<n; i++)
			if (p->getFolderIndexInfo(i).marked)
				++cnt;


		if (!getTag(Gettext(_("Tag %1% messages as: ")) << cnt,
			    tagNum))
			return true;

		vector<size_t> msgArray;

		n=p->getFolderIndexSize();

		for (i=0; i<n; i++)
			if (p->getFolderIndexInfo(i).marked)
				msgArray.push_back(i);
		folder->setTag(msgArray, tagNum);

		return true;
	}

	if (key_TOGGLE == ka[0])
	{
		vector<size_t> msgNums;

		size_t n=p->getFolderIndexSize();

		size_t i;

		for (i=0; i<n; i++)
			msgNums.push_back(i);

		myServer::Callback callback;

		mail::messageInfo flags;

		flags.marked=true;

		p->updateFolderIndexFlags(msgNums, true, false, flags,
					  callback);

		myServer::eventloop(callback);
		return true; // We're done.
	}
	return false;
}

bool CursesIndexDisplay::watchPrompt(unsigned &nDays, unsigned &nLevels)
{
	nDays=Watch::defaultWatchDays;
	nLevels=Watch::defaultWatchDepth;

	for (;;)
	{
		myServer::promptInfo prompt=
			myServer
			::promptInfo(Gettext(_("Watch thread up to %1% days, "
					       "%2%-deep replies? (Y/N) "))
				     << nDays
				     << nLevels).yesno()
			.option(key_WATCHDAYS,
				Gettext::keyname(_("WATCHDAYS_K:D")),
				_("Days"))
			.option(key_WATCHDEPTH,
				Gettext::keyname(_("WATCHDEPTH_K:R")),
				_("Replies"));


		prompt=myServer::prompt(prompt);

		if (prompt.abortflag || myServer::nextScreen)
			return false;

		vector<wchar_t> ka;

		Curses::mbtow( ((string)prompt).c_str(), ka);
		if (ka.size())
		{
			if (key_WATCHDAYS == ka[0])
			{
				ostringstream o;

				o << nDays;

				prompt=myServer::promptInfo(_("Days: "))
					.initialValue(o.str());

				prompt=myServer::prompt(prompt);

				if (prompt.abortflag || myServer::nextScreen)
					return false;

				istringstream i((string)prompt);

				i >> nDays;

				if (nDays <= 0)
					nDays=1;
				if (nDays > 99)
					nDays=99;
				continue;
			}

			if (key_WATCHDEPTH == ka[0])
			{
				ostringstream o;

				o << nLevels;

				prompt=myServer::promptInfo(_("Replies: "))
					.initialValue(o.str());

				prompt=myServer::prompt(prompt);
				if (prompt.abortflag || myServer::nextScreen)
					return false;

				istringstream i((string)prompt);

				i >> nLevels;

				if (nLevels <= 0)
					nLevels=1;
				if (nLevels > 99)
					nLevels=99;
				continue;
			}
		}

		if ( (string)prompt != "Y")
			return false;

		break;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////

bool CursesIndexDisplay::processKey(const Curses::Key &key)
{
	myFolder *f=getFolderIndex();

	if (!f)
		return false;

	if (GlobalKeys::processKey(key, GlobalKeys::FOLDERINDEXSCREEN,
				   f->getServer()))
		return true;

	if (key == key_SORTFOLDER)
	{
		bool notFlag=false;

		for (;;)
		{
			myServer::promptInfo prompt=
				myServer
				::promptInfo(notFlag ?
					     _("Sort folder by: (DESC) "):
					     _("Sort folder by: "))
				.option(key_SORTFOLDER_ARRIVAL,
					Gettext
					::keyname(_("SORTFOLDER_ARRIVAL_K:A")),
					_("by recv date"))
				.option(key_SORTFOLDER_DATE,
					Gettext
					::keyname(_("SORTFOLDER_DATE_K:D")),
					_("by sent date"))
				.option(key_SORTFOLDER_NAME,
					Gettext
					::keyname(_("SORTFOLDER_NAME_K:N")),
					_("by name"))
				.option(key_SORTFOLDER_SUBJECT,
					Gettext
					::keyname(_("SORTFOLDER_SUBJECT_K:S")),
					_("by subject"))
				.option(key_SORTFOLDER_THREAD,
					Gettext
					::keyname(_("SORTFOLDER_THREAD_K:T")),
					_("by thread"))
				.option(key_SORTNOT,
					Gettext
					::keyname(_("SORTNOT_K:!")),
					_("Asc/Desc"));

			prompt=myServer::prompt(prompt);
			if (prompt.abortflag)
				return true;

			vector<wchar_t> ka;

			Curses::mbtow( ((string)prompt).c_str(), ka);
			if (ka.size() == 0)
				return true;
			wchar_t k=ka[0];

			if (key_SORTNOT == k)
			{
				notFlag= !notFlag;
				continue;
			}

			if (key_SORTFOLDER_ARRIVAL == k)
				f->setSortFunction(notFlag ? "!A":"A");
			if (key_SORTFOLDER_THREAD == k)
				f->setSortFunction(notFlag ? "!T":"T");
			if (key_SORTFOLDER_DATE == k)
				f->setSortFunction(notFlag ? "!D":"D");
			if (key_SORTFOLDER_SUBJECT == k)
				f->setSortFunction(notFlag ? "!S":"S");
			if (key_SORTFOLDER_NAME == k)
				f->setSortFunction(notFlag ? "!N":"N");
			break;
		}
		return true;
	}

	if (key == key_BLOBS)
	{
		myFolder *f=getFolderIndex();
		myServer *s=f->getServer();
		string b=s->getFolderConfiguration(f->getFolder(),
						   "INDEXBLOBS");

		b= b == "1" ? "0":"1";

		s->updateFolderConfiguration(f->getFolder(), "INDEXBLOBS", b);
		s->saveFolderConfiguration();
		draw();
		return true;
	}

	if (key == key_VIEWATT)
	{
		f->viewAttachments(f->getCurrentMessage());
		return true;
	}

	return false;
}


int CursesIndexDisplay::getCursorPosition(int &row, int &col)
{
	myFolder *f=getFolderIndex();

	row=f ? f->getCurrentMessage():0;
	col=0;
	Curses::getCursorPosition(row, col);
	return 0;
}


bool CursesIndexDisplay::listKeys( vector< pair<string, string> > &list)
{
	GlobalKeys::listKeys(list, GlobalKeys::FOLDERINDEXSCREEN);

	list.push_back(make_pair(Gettext::keyname(_("NEXT_K:N")),
				 _("Next")));

	list.push_back(make_pair(Gettext::keyname(_("PREV_K:P")),
				 _("Prev")));

	list.push_back( make_pair(Gettext::keyname(_("SEARCH_K:;")),
				  _("Search")));

	list.push_back( make_pair(Gettext::keyname(_("SORTFOLDER_K:$")),
				  _("Sort folder")));

	list.push_back( make_pair(Gettext::keyname(_("WATCH_K:A")),
				 _("wAtch/Unwatch")));

	list.push_back(make_pair(Gettext::keyname(_("BOUNCE_K:B")),
				 _("Blind Fwd")));

	list.push_back( make_pair(Gettext::keyname(_("COPY_K:C")),
				 _("Copy")));

	list.push_back( make_pair(Gettext::keyname(_("MOVE_K:O")),
				 _("mOve")));

	list.push_back( make_pair(Gettext::keyname(_("DELETE_K:D")),
				 _("Delete")));

	list.push_back( make_pair(Gettext::keyname(_("FWD_K:F")),
				 _("Fwd")));

	list.push_back( make_pair(Gettext::keyname(_("JUMP_K:J")),
				  _("Jump To")));

	list.push_back( make_pair(Gettext::keyname(_("REPLY_K:R")),
				  _("Reply")));

	list.push_back( make_pair(Gettext::keyname(_("TOGGLEMARK_K:SP")),
				  _("Tag/Untag")));

	list.push_back(make_pair(Gettext::keyname(_("VIEWATT_K:V")),
				 _("View Attchs")));

	list.push_back( make_pair(Gettext::keyname(_("UNDELETE_K:U")),
				 _("Undelete/Unread")));

	list.push_back( make_pair(Gettext::keyname(_("EXPUNGE_K:X")),
				 _("Expunge deleted")));

	list.push_back( make_pair(Gettext::keyname(_("TAG_K:T")),
				  _("Tag")));
	list.push_back( make_pair(Gettext::keyname(_("BLOBS_K:S")),
				  _("Show blobs")));
	return false;
}

//////////////////////////////////////////////////////////////////////////
//
// CheckNewMail does nothing noteworthy - the heavy lifting is trigged in
// myFolder::newMessages()
//

CursesIndexDisplay::CheckNewMail::CheckNewMail()
{
}

CursesIndexDisplay::CheckNewMail::~CheckNewMail()
{
}

void CursesIndexDisplay::CheckNewMail::success(string message)
{
	delete this;
}

void CursesIndexDisplay::CheckNewMail::fail(string message)
{
	delete this;
}
