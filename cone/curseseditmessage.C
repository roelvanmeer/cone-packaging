/* $Id: curseseditmessage.C,v 1.10 2005/02/24 03:39:09 mrsam Exp $
**
** Copyright 2003-2005, Double Precision Inc.
**
** See COPYING for distribution information.
*/
#include "config.h"
#include "curseseditmessage.H"
#include "curses/cursesstatusbar.H"
#include "curses/cursesmainscreen.H"
#include "curses/cursesmoronize.H"
#include "gettext.H"
#include "wraptext.H"
#include "htmlentity.h"
#include "myserverpromptinfo.H"
#include "macros.H"
#include "maildir/maildirsearch.h"
#include "unicode/unicode.h"
#include "opendialog.H"
#include "libmail/mail.H"

#include <fstream>
#include <vector>

#include <errno.h>
#include <string.h>
#include <ctype.h>

#if HAVE_ISWALPHA
#include <wctype.h>
#endif

using namespace std;

extern Gettext::Key key_ALL;
extern Gettext::Key key_CUT;
extern Gettext::Key key_JUSTIFY;
extern Gettext::Key key_EDITSEARCH;
extern Gettext::Key key_EDITREPLACE;
extern Gettext::Key key_GETFILE;
extern Gettext::Key key_DICTSPELL;
extern Gettext::Key key_EXTEDITOR;

extern Gettext::Key key_REPLACE0;
extern Gettext::Key key_REPLACE1;
extern Gettext::Key key_REPLACE2;
extern Gettext::Key key_REPLACE3;
extern Gettext::Key key_REPLACE4;
extern Gettext::Key key_REPLACE5;
extern Gettext::Key key_REPLACE6;
extern Gettext::Key key_REPLACE7;
extern Gettext::Key key_REPLACE8;
extern Gettext::Key key_REPLACE9;

extern Gettext::Key key_ABORT;
extern Gettext::Key key_MACRO;

extern Gettext::Key key_REPLACE_K;
extern Gettext::Key key_IGNORE_K;
extern Gettext::Key key_IGNOREALL_K;

extern char larr[], rarr[];

extern CursesStatusBar *statusBar;
extern CursesMainScreen *mainScreen;
extern unicodeEntityAltList *currentEntityAltList;
extern SpellCheckerBase *spellCheckerBase;

time_t CursesEditMessage::autosaveInterval=60;
string CursesEditMessage::externalEditor;

////////////////////////////////////////////////////////////////////////////
//
// Helper class instantiated when spell checking begins.
// When this object goes out of scope, the spell checker object gets
// automatically cleaned up.
//

class CursesEditMessage::SpellCheckerManager {

public:
	SpellCheckerBase::Manager *manager;

	SpellCheckerManager();
	~SpellCheckerManager();

	operator SpellCheckerBase::Manager *();

	SpellCheckerBase::Manager *operator ->();
};

CursesEditMessage::SpellCheckerManager::SpellCheckerManager()
	: manager(NULL)
{
}

CursesEditMessage::SpellCheckerManager::~SpellCheckerManager()
{
	if (manager)
		delete manager;
}

CursesEditMessage::SpellCheckerManager::operator SpellCheckerBase::Manager *()
{
	return manager;
}

SpellCheckerBase::Manager *CursesEditMessage::SpellCheckerManager::operator->()
{
	return manager;
}

///////////////////////////////////////////////////////////////////////////
//
// Helper class for defining a macro.  A key handler that intercepts function
// keypresses and aborts the "New Macro Name" prompt.  Pressing CTRL-N
// brings up a "New Macro Name" prompt.  Either enter a new shortcut for the
// macro, or press a function key (which this helper class intercepts).

class CursesEditMessage::DefineMacroHelper : public CursesKeyHandler {

public:

	bool defineFkeyFlag;
	int fkeyNum;

	DefineMacroHelper();
	~DefineMacroHelper();

	bool processKey(const Curses::Key &key);
};

CursesEditMessage::DefineMacroHelper::DefineMacroHelper()
	: CursesKeyHandler(PRI_STATUSHANDLER),
	  defineFkeyFlag(false), fkeyNum(0)
{
}

CursesEditMessage::DefineMacroHelper::~DefineMacroHelper()
{
}

bool CursesEditMessage::DefineMacroHelper::processKey(const Curses::Key &key)
{
	if (key.fkey())
	{
		defineFkeyFlag=true;
		fkeyNum=key.fkeynum();
		statusBar->fieldAbort();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////
//
// When yank-ing back a text buffer, the helper object provides the yanked
// text in UTF-8 format.  Used for regular yanks, and macro key hits.

class CursesEditMessage::yankHelper {
public:
	yankHelper();
	virtual ~yankHelper();

	virtual operator std::string()=0;

	class cut; // A regular cut operator, obtain from cutBuffer

	class macro; // Insert a macro.
};

CursesEditMessage::yankHelper::yankHelper()
{
}

CursesEditMessage::yankHelper::~yankHelper()
{
}

class CursesEditMessage::yankHelper::cut:public CursesEditMessage::yankHelper {
public:
	cut();
	~cut();

	operator std::string();
};

CursesEditMessage::yankHelper::cut::cut()
{
}

CursesEditMessage::yankHelper::cut::~cut()
{
}

CursesEditMessage::yankHelper::cut::operator std::string()
{
	string s;

	char *p=unicode_ctoutf8( Gettext::defaultCharset(),
				 CursesField::cutBuffer
				 .c_str(), NULL);

	if (!p)
		outofmemory();

	try {
		s=p;
		free(p);
	} catch (...) {
		free(p);
		LIBMAIL_THROW();
	}
	return s;
}


class CursesEditMessage::yankHelper::macro
	:public CursesEditMessage::yankHelper {

	string &textPtr;
public:
	macro(string &textPtrArg);
	~macro();

	operator std::string();
};


CursesEditMessage::yankHelper::macro::macro(string &textPtrArg)
	: textPtr(textPtrArg)
{
}

CursesEditMessage::yankHelper::macro::~macro()
{
}

CursesEditMessage::yankHelper::macro::operator string()
{
	return textPtr;
}

CursesEditMessage::CursesEditMessage(CursesContainer *parent)
	: Curses(parent), CursesKeyHandler(PRI_SCREENHANDLER),
	  cursorCol(0),
	  cursorLineHorizShift(0), marked(false),
	  lastKey((wchar_t)0)
{
	text_UTF8.push_back(""); // Always have at least one line in text_UTF8
	cursorRow.me=this;
}

CursesEditMessage::~CursesEditMessage()
{
}

void CursesEditMessage::cursorRowChanged()
{
}

int CursesEditMessage::getWidth() const
{
	return getScreenWidth();
}

int CursesEditMessage::getHeight() const
{
	size_t n=text_UTF8.size();

	size_t h=getScreenHeight();

	if (h > n)
		n=h;

	return n;
}

bool CursesEditMessage::isFocusable()
{
	return true;
}

//
// Get a line of text as unicode characters
//

void CursesEditMessage::getText(size_t line,
				vector<unicode_char> &textRef)
{
	textRef.clear();

	if (line >= numLines())
		return;

	string l=line_utf8(line);

	unicode_char *ucp=unicode_fromutf8(l.c_str());

	if (!ucp)
		outofmemory();

	try {
		size_t i;

		for (i=0; ucp[i]; i++)
			;

		textRef.insert(textRef.end(), ucp, ucp+i);
		free(ucp);
	} catch (...) {
		free(ucp);
		LIBMAIL_THROW();
	}
}

//
// Save line of text as unicode chars.
//

void CursesEditMessage::setText(size_t line,
				vector<unicode_char> &textRef)
{
	if (line >= numLines())
		abort();

	vector<unicode_char> buf=textRef;

	buf.push_back(0);

	char *p=unicode_toutf8(&buf[0]);

	if (!p)
		outofmemory();

	try {
		text_UTF8[line]=p;
		free(p);
	} catch (...) {
		free(p);
		LIBMAIL_THROW();
	}
}

bool CursesEditMessage::processKeyInFocus(const Key &key)
{
	Key lastKeyProcessed=lastKey;

	lastKey=key;

	if (cursorRow >= numLines())
		abort();

	if (key == key.SHIFTLEFT || key == key.SHIFTRIGHT ||
	    key == key.SHIFTUP || key == key.SHIFTDOWN ||
	    key == key.SHIFTHOME || key == key.SHIFTEND ||
	    key == key.SHIFTPGUP || key == key.SHIFTPGDN)
	{
		if (!marked)
			mark();
	}
	else if (marked && key != key.DEL && key != key.BACKSPACE &&
		 key != key_CUT && key != key.CLREOL && key != key_MACRO)
	{
		marked=false;
		draw();
	}

	if (key == key.UP || key == key.SHIFTUP)
	{
		if (cursorRow == 0)
			transferPrevFocus();
		else
		{
			--cursorRow;
			drawLine(cursorRow+1);
		}
		drawLine(cursorRow);
		return true;
	}

	if (key == key.DOWN || key == key.SHIFTDOWN)
	{
		if (cursorRow + 1 >= numLines())
			transferNextFocus();
		else
		{
			++cursorRow;
			drawLine(cursorRow-1);
		}
		drawLine(cursorRow);
		return true;
	}

	if (key == key.LEFT || key == key.SHIFTLEFT)
	{
		left(true);
		return true;
	}

	if (key == key.RIGHT || key == key.SHIFTRIGHT)
	{
		right();
		return true;
	}

	if (key == key.HOME || key == key.SHIFTHOME)
	{
		cursorCol=0;
		cursorLineHorizShift=0;
		drawLine(cursorRow);
		return true;
	}

	if (key == key.END || key == key.SHIFTEND)
	{
		end();
		return true;
	}

	if (key == key.PGUP || key == key.SHIFTPGUP)
	{
		return Curses::processKeyInFocus(key);
#if 0
		size_t dummy;
		size_t h;

		getVerticalViewport(dummy, h);

		drawLine(cursorRow);
		cursorRow -= cursorRow >= h ? h:cursorRow;
		drawLine(cursorRow);
		return true;
#endif
	}

	if (key == key.PGDN || key == key.SHIFTPGDN)
	{
		return Curses::processKeyInFocus(key);
	}

	if (key == key.ENTER)
	{

		// Get current line

		vector<unicode_char> currentLine, nextLine;

		getText(cursorRow, currentLine);

		// Find character cursor is at.

		vector<unicode_char>::iterator pos
			=getIndexOfCol(currentLine, cursorCol);

		// Split the line at that point.

		nextLine.insert(nextLine.end(), pos, currentLine.end());

		currentLine.erase(pos, currentLine.end());

		// Insert a row.
		text_UTF8.insert(text_UTF8.begin() + cursorRow + 1, "");

		// Update both rows.
		setText(cursorRow + 1, nextLine);
		setText(cursorRow, currentLine);

		++cursorRow;
		cursorCol=0;
		draw();
		modified();
		return true;
	}

	if (key == key.BACKSPACE)
	{
		left(false);
		deleteChar();
		return true;
	}

	if (key == key.DEL || (marked && key == key_CUT))
	{
		deleteChar();
		return true;
	}

	if (key == key_JUSTIFY)
	{
		modified();

		if (lastKeyProcessed == key_JUSTIFY) // Undo justification
		{
			vector<string> restoredParagraph;

			size_t r;

			// Previously unjustified text is in the cut buffer.
			// Cut buffer uses native charset.  We're dealing
			// with utf-8.

			while ((r=CursesField::cutBuffer.find('\n')) !=
			       CursesField::cutBuffer.npos)
			{
				string s=CursesField::cutBuffer.substr(0, r);

				char *p=unicode_ctoutf8(Gettext::
							defaultCharset(),
							s.c_str(), NULL);

				if (!p)
					outofmemory();

				try {
					s=p;
					free(p);
				} catch (...) {
					free(p);
					LIBMAIL_THROW();
				}

				restoredParagraph.push_back(s);

				CursesField::cutBuffer=CursesField::cutBuffer
					.substr(r+1);
			}

			if (restoredParagraph.size() == 0)
				return true;

			// Find where the justified paragraph begins.

			r=cursorRow;

			while (r > 0)
			{
				--r;

				string line=text_UTF8[r];

				if (line.size() == 0 || line[0] == '>')
				{
					++r;
					break;
				}
			}

			erase(); // Clear the screen.

			// Now restore the text.
			text_UTF8.erase(text_UTF8.begin() + r,
					text_UTF8.begin()+cursorRow);
			text_UTF8.insert(text_UTF8.begin() + r,
					 restoredParagraph.begin(),
					 restoredParagraph.end());

			// Figure out where the cursor should be now,
			// redraw the screen.
			cursorRow=r+restoredParagraph.size() - 1;
			cursorCol=0;
			CursesField::cutBuffer="";
			lastKey=Key((wchar_t)0); // Not really
			draw();
			return true;
		}

		// We can only justify if the cursor is on a non-empty
		// line that does not begin with quoted text.


		if (cursorRow >= text_UTF8.size())
		{
			lastKey=Key((wchar_t)0); // Not really
			return true;
		}

		string line=text_UTF8[cursorRow];

		if (line.size() == 0)
		{
			lastKey=Key((wchar_t)0); // Not really
			processKeyInFocus(Key(Key::DOWN)); // Easier
			return true;
		}

		if (line[0] == '>')
		{
			lastKey=Key((wchar_t)0); // Not really
			statusBar->beepError();
			return true;
		}

		// Find start of paragraph

		while (cursorRow > 0)
		{
			line=text_UTF8[--cursorRow];

			if (line.size() == 0 || line[0] == '>')
			{
				++cursorRow;
				break;
			}
		}

		vector<unicode_char> paragraph;

		size_t startingRow=cursorRow;

		CursesField::cutBuffer="";
		// Original, unjustified text goes here.

		const struct unicode_info *myCharset=
			Gettext::defaultCharset();

		while (cursorRow < text_UTF8.size())
		{
			line=text_UTF8[cursorRow];

			if (line.size() == 0 || line[0] == '>')
				break; // Done, next paragraph.

			char *p=unicode_cfromutf8(myCharset,
						  line.c_str(), NULL);

			if (!p)
				outofmemory();
			try {
				line=p;
				free(p);
			} catch (...) {
				free(p);
			}

			++cursorRow;
			string::iterator b, c, e;

			CursesField::cutBuffer += line;
			CursesField::cutBuffer += "\n";

			b=line.begin();
			e=line.end();

			for (c=b; b != e; )
				if (!isspace((int)(unsigned char)*b++))
					c=b;

			line.erase(c, e);

			if (paragraph.size() > 0)
				paragraph.push_back(' ');

			unicode_char *uc= (myCharset->c2u)(myCharset,
							   line.c_str(), NULL);

			if (!uc)
				LIBMAIL_THROW((strerror(errno)));

			try {
				size_t i;

				for (i=0; uc[i]; i++)
					;

				paragraph.insert(paragraph.end(),
						 uc, uc+i);

				free(uc);
			} catch (...) {
				free(uc);
				LIBMAIL_THROW();
			}
		}

		erase();
		text_UTF8.erase(text_UTF8.begin() + startingRow,
				text_UTF8.begin() + cursorRow);
		cursorRow=startingRow;
		cursorCol=0;

		vector<string> reformattedParagraph=WrapText(paragraph,
							     LINEW);

		paragraph.clear();

		vector<string>::iterator b, e;

		b=reformattedParagraph.begin();
		e=reformattedParagraph.end();

		bool lastLine=true;

		while (b != e)
		{
			--e;
			if (lastLine)
				lastLine=false;
			else
				*e += " ";


			char *p=unicode_ctoutf8(myCharset,
						e->c_str(), NULL);

			if (!p)
				outofmemory();

			try {
				*e=p;
				free(p);
			} catch (...) {
				free(p);
				LIBMAIL_THROW();
			}
		}

		text_UTF8.insert(text_UTF8.begin() + cursorRow,
				 reformattedParagraph.begin(),
				 reformattedParagraph.end());
		cursorRow = cursorRow + reformattedParagraph.size();
		if (cursorRow >= text_UTF8.size())
			cursorRow=text_UTF8.size()-1;

		marked=false;
		draw();
		statusBar->status(_("^J: undo justification"));
		return true;
	}

	if (key == key.CLREOL)
	{
		string saveCutBuffer=CursesField::cutBuffer;

		if (lastKeyProcessed != key.CLREOL)
			saveCutBuffer="";

		CursesField::cutBuffer="";
		mark();
		end();
		deleteChar();

		if (CursesField::cutBuffer.size() == 0)
			CursesField::cutBuffer="\n"; // Must be

		CursesField::cutBuffer=saveCutBuffer + CursesField::cutBuffer;
		return true;
	}

	if (key == key.SHIFT)
	{
		statusBar->clearstatus();
		if (shiftmode)
			statusBar->status(_("Mark set"));

		return true;
	}

	if (key == key_EDITSEARCH || key == key_EDITREPLACE)
	{
		const struct unicode_info &myCharset=
			*Gettext::defaultCharset();

		MONITOR(CursesEditMessage);

		myServer::promptInfo response=
			myServer
			::prompt(myServer
				 ::promptInfo(Gettext(_("Search: ")))
				 .initialValue(defaultSearchStr));

		if (DESTROYED() || response.abortflag ||
		    response.value.size() == 0)
			return true;

		defaultSearchStr=response.value;

		// We match in UTF-8, convert the search string to UTF8,
		// and initialize the search engine.

		mail::Search searchEngine;

		bool doSmartReplace=true;

		// If the search string is completely in lowercase, do a
		// "smart" replace (replace wholly uppercase strings with
		// uppercase replacement text, and replace title-cased string
		// with title-cased replacement text).

		size_t j;
		{
			unicode_char *uc= (*myCharset.c2u)
				(&myCharset, defaultSearchStr.c_str(), NULL);

			if (!uc)
				outofmemory();

			size_t i;

			for (i=j=0; uc[i]; )
			{
				// A single whitespace in the search string
				// matches any amount of whitespace in text,
				// therefore collapse consecutive whitespace
				// characters into a single space.

				bool whiteSpace=false;

				while (uc[i] && (unsigned char)uc[i] == uc[i]
				       && isspace(uc[i]))
				{
					whiteSpace=true;
					i++;
				}

				if (whiteSpace)
					uc[j++]=' ';

				if (uc[i])
				{
					if (uc[i] != unicode_lc(uc[i]))
						doSmartReplace=false;
					uc[j++]=unicode_uc(uc[i++]);
				}

			}
			uc[j]=0;

			try {
				char *p= (*unicode_UTF8.u2c)(&unicode_UTF8,
							     uc, NULL);

				if (!p)
					outofmemory();

				try {
					if (searchEngine.setString(p) < 0)
						outofmemory();
					free(p);
				} catch (...) {
					free(p);
					LIBMAIL_THROW();
				}

				free(uc);
			} catch (...) {
				free(uc);
				LIBMAIL_THROW();
			}

			if (j == 0)
				return false; // empty search string
		}

		if (key == key_EDITSEARCH) // A single search
		{
			search(true, true, searchEngine, j);
			return true;
		}

		// Search/replace

		response=myServer
			::prompt(myServer
				 ::promptInfo(Gettext(_("Replace: ")))
				 .initialValue(defaultReplaceStr));

		if (DESTROYED() || response.abortflag)
			return true;

		defaultReplaceStr=response.value;

		size_t replaceCnt=0;
		bool globalReplace=false;

		while (search(!globalReplace, false, searchEngine, j))
		{
			if (!globalReplace)
			{
				response=
					myServer
					::promptInfo(Gettext(_("Replace"
							       "? (Y/N) ")))
					.yesno()
					.option( key_ALL,
						 Gettext::keyname(_("ALL_K:A")
								  ),
						 _("Replace All"));

				response=myServer::prompt(response);

				if (DESTROYED() || response.abortflag)
					return true;

				vector<wchar_t> ka;

				Curses::mbtow( ((string)response).c_str(), ka);
				if (ka.size() > 0 &&
				    (key_ALL == ka[0]))
					globalReplace=true;
				else if (response.value != "Y")
					continue;
			}

			CursesField::cutBuffer="";
			deleteChar(); // Found search string is marked.

			string replaceStr=defaultReplaceStr;

			if (doSmartReplace)
			{
				unicode_char *uc=(myCharset.c2u)
					(&myCharset,
					 CursesField::cutBuffer.c_str(), NULL);

				if (uc)
				{
					size_t j;

					for (j=0; uc[j]; j++)
						if (unicode_uc(uc[j]) != uc[j])
							break;

					bool toUpper= uc[j] == 0;
					bool toTitle=
						uc[0] == unicode_tc(uc[0]);

					char *p=NULL;

					free(uc);

					if (toUpper)
					{
						p=(*myCharset.toupper_func)
							(&myCharset,
							 replaceStr.c_str(),
							 NULL);
					}
					else if (toTitle)
					{
						uc=(*myCharset.c2u)
							(&myCharset,
							 replaceStr.c_str(),
							 NULL);
						if (uc)
						{
							uc[0]=unicode_tc(uc[0]
									 );

							p=unicode_fromCharset(&myCharset,
									      uc,
									      currentEntityAltList);
							free(uc);
						}
					}

					if (p)
						try {
							replaceStr=p;
							free(p);
						} catch (...) {
							free(p);
							LIBMAIL_THROW();
						}
				}

			}

			CursesField::cutBuffer=replaceStr;

			{
				CursesEditMessage::yankHelper::cut c;

				yank(CursesField::yankKey, false, c);
			}
			++replaceCnt;
		}
		draw();
		statusBar->clearstatus();
		statusBar->status(Gettext(_("Replaced %1% occurences."))
				  << replaceCnt);
		return true;
	}

	if (key == key_GETFILE)
	{
		string filename;

		{
			OpenDialog open_dialog;

			open_dialog.noMultiples();

			open_dialog.requestFocus();
			myServer::eventloop();

			vector<string> &filenameList=
				open_dialog.getFilenameList();

			if (filenameList.size() == 0)
				filename="";
			else
				filename=filenameList[0];

			mainScreen->erase();
		}
		mainScreen->draw();
		requestFocus();

		if (filename.size() == 0)
			return true;

		vector<string> filebuffer;

		ifstream i(filename.c_str());

		if (i.fail())
		{
			statusBar->clearstatus();
			statusBar->status(strerror(errno),
					  statusBar->SYSERROR);
			statusBar->beepError();
			return true;
		}

		for (;;)
		{
			string line;

			getline(i, line);

			if (i.fail() && !i.eof())
				break;

			if (i.eof() && line.size() == 0)
				break;

			modified();

			string::iterator b=line.begin(), e=line.end(), c=b;

			while (b != e)
			{
				if (*b == 0 && filebuffer.size() == 0)
				{
					statusBar->clearstatus();
					statusBar->status(_("Binary file? You must be joking."),
							  statusBar->SYSERROR);
					statusBar->beepError();
					return true;
				}

				if (((int)(unsigned char)*b) < ' ' &&
				    *b != '\t')
				{
					b++;
					continue;
				}
				*c++ = *b++;
			}

			line.erase(c, e);

			char *p=unicode_ctoutf8(Gettext::defaultCharset(),
						line.c_str(), NULL);

			if (!p)
				outofmemory();

			try {
				line=p;
				free(p);
			} catch (...) {
				free(p);
				LIBMAIL_THROW();
			}

			filebuffer.push_back(line);
		}

		if (!i.eof())
		{
			statusBar->clearstatus();
			statusBar->status(strerror(errno),
					  statusBar->SYSERROR);
			statusBar->beepError();
			return true;
		}

		if (cursorRow < text_UTF8.size())
			text_UTF8.insert(text_UTF8.begin() + cursorRow,
					 filebuffer.begin(), filebuffer.end());
		else
			text_UTF8.insert(text_UTF8.end(),
					 filebuffer.begin(),
					 filebuffer.end());
		draw();
		return true;
	}

	if (key == key_DICTSPELL)
	{
		vector<unicode_char> currentLine;
		size_t row=cursorRow;

		getText(row, currentLine);

		size_t pos=getIndexOfCol(currentLine, cursorCol)
			- currentLine.begin();

		string errmsg;

		SpellCheckerBase::Manager *managerPtr=
			spellCheckerBase->getManager(errmsg);

		if (!managerPtr)
		{
			statusBar->status(errmsg);
			statusBar->beepError();
			return true;
		}

		SpellCheckerManager manager;

		manager.manager=managerPtr;

		MONITOR(CursesEditMessage);

		bool needPrompt=true;

		while (!DESTROYED())
		{
			if (needPrompt)
			{
				statusBar->clearstatus();
				statusBar->status(_("Spell checking..."));
				statusBar->flush();
				needPrompt=false;
			}

			if ((currentLine.size() > 0 && currentLine[0] == '>')
			    // Do not spellcheck quoted content
			    ||
			    pos >= currentLine.size())
			{
				if (++row >= text_UTF8.size())
					break;

				currentLine.clear();
				getText(row, currentLine);
				pos=0;
				continue;
			}

			// Grab the next word

			size_t i=pos;

			while (i < currentLine.size())
			{
#if HAVE_ISWALPHA
				if (!iswalpha(currentLine[i]))
					break;
#else
				unicode_char u=currentLine[i];

				if (u <= 0x00FF && !isalpha(u))
					break;
#endif

				i++;
			}

			if (i == pos)
			{
				++pos;
				continue;
			}

			// Convert word to utf-8

			string word_c;

			{
				vector<unicode_char> word;

				word.insert(word.end(),
					    currentLine.begin()+pos,
					    currentLine.begin()+i);
				word.push_back(0);

				char *p=(*unicode_UTF8.u2c)(&unicode_UTF8,
							    &word[0], NULL);

				if (p)
					try {
						word_c=p;
						free(p);
					} catch (...) {
						free(p);
						LIBMAIL_THROW();
					}
			}

			bool found_flag;
			string errmsg;

			// Spell check.

			if (word_c.size() == 0 ||
			    (errmsg=manager->search(word_c, found_flag))
			    .size() > 0)
			{
				statusBar->clearstatus();
				statusBar->status(errmsg);
				statusBar->beepError();
				break;
			}

			if (found_flag)
			{
				pos=i;
				continue;
			}

			// Highlight misspelled word

			cursorRow=row;

			vector<size_t> positions;

			getTextPos(currentLine, positions);
			cursorCol=positions[pos];
			mark();

			int dummy1, dummy2;
			getCursorPosition(dummy1, dummy2);
			// Make sure starting position is shown.

			cursorCol=positions[i];
			drawLine(cursorRow);

			bool doReplace;
			string origWord=word_c;

			needPrompt=true;

			if (!checkReplace(doReplace, word_c, manager.manager))
			{
				marked=false;
				drawLine(row);
				return true;
			}

			unicode_char *uc;

			if (doReplace &&
			    (uc=(*unicode_UTF8.c2u)(&unicode_UTF8,
						    word_c.c_str(), NULL))
			    != NULL)
			{
				try {
					int n;

					modified();

					for (n=0; uc[n]; n++)
						;

					currentLine
						.erase(currentLine.begin()
						       + pos,
						       currentLine.begin()+i);

					currentLine.insert(currentLine.begin()
							   + pos, uc, uc+n);

					setText(row, currentLine);

					pos += n;

					vector<size_t> spos;
					getTextPos(currentLine, spos);
					cursorCol=spos[pos];
					free(uc);
				} catch (...) {
					free(uc);
					LIBMAIL_THROW();
				}

				manager->replace(origWord, word_c);
			}
			else
			{
				pos=i;
			}
			marked=false;
			drawLine(row);
		}

		statusBar->status(_("Spell checking completed."));
		return true;
	}

	if (key == key_EXTEDITOR)
	{
		if (externalEditor.size() == 0)
			return true; // leaf

		statusBar->clearstatus();
		statusBar->status(_("Starting editor..."));
		statusBar->flush();

		string msgfile=getConfigDir() + "/message.tmp";

		ofstream o(msgfile.c_str());

		if (!o.is_open())
		{
			statusBar->clearstatus();
			statusBar->status(strerror(errno));
			statusBar->beepError();
			return true;
		}

		const struct unicode_info *uc=Gettext::defaultCharset();

		std::vector<std::string>::iterator b, e;

		b=text_UTF8.begin();
		e=text_UTF8.end();

		while (b != e)
		{
			string s= *b++;

			char *p=unicode_cfromutf8(uc, s.c_str(), NULL);

			if (!p)
				outofmemory();

			o << p << endl;
			free(p);
		}

		o.flush();
		if (o.fail() || o.bad())
		{
			statusBar->clearstatus();
			statusBar->status(strerror(errno));
			statusBar->beepError();
			return true;
		}
		o.close();

		vector<const char *> args;

		{
			const char *p=getenv("SHELL");

			if (!p || !*p)
				p="/bin/sh";

			args.push_back(p);
		}

		args.push_back("-c");

		int rc;

		{
			string cmd=externalEditor + " " + msgfile;

			args.push_back(cmd.c_str());
			args.push_back(0);

			rc=Curses::runCommand(args, -1, "");
		}
		extedited();
		if (rc)
		{
			unlink(msgfile.c_str());

			statusBar->clearstatus();
			statusBar->
				status(Gettext(_("The external editor command "
						 "failed.  Check that the "
						 "configured external editor, "
						 "\"%1%\", exists."))
				       << externalEditor);
			statusBar->beepError();
			return true;
		}

		ifstream i(msgfile.c_str());

		if (!i.is_open())
		{
			unlink(msgfile.c_str());
			statusBar->clearstatus();
			statusBar->status(strerror(errno));
			statusBar->beepError();
			return true;
		}

		text_UTF8.clear();
		cursorRow=0;
		draw();

		for (;;)
		{
			string line;

			getline(i, line);

			if (i.fail() && !i.eof())
				break;

			if (i.eof() && line.size() == 0)
				break;

			string::iterator b=line.begin(), e=line.end(), c=b;

			while (b != e)
			{
				if (((int)(unsigned char)*b) < ' ' &&
				    *b != '\t')
				{
					b++;
					continue;
				}
				*c++ = *b++;
			}

			line.erase(c, e);

			char *p=unicode_ctoutf8(Gettext::defaultCharset(),
						line.c_str(), NULL);

			if (!p)
				outofmemory();

			try {
				line=p;
				free(p);
			} catch (...) {
				free(p);
				LIBMAIL_THROW();
			}

			text_UTF8.push_back(line);
		}
		i.close();
		unlink(msgfile.c_str());
		if (text_UTF8.size() == 0)
			text_UTF8.push_back("");
		draw();
		modified();
		return true;
	}

	if (key == key_MACRO)
	{
		Macros *m=Macros::getRuntimeMacros();

		if (!m)
			return true; // Leaf

		if (!marked)
		{
			statusBar->clearstatus();
			statusBar->status(_("Mark a section of text first."));
			statusBar->beepError();
			return true;
		}

		myServer::promptInfo macroNamePrompt=
			myServer::promptInfo(_("Macro shortcut: "));

		macroNamePrompt.optionHelp
			.push_back(make_pair(Gettext
					     ::keyname(_("ENTER:Enter")),
					     _("Define shortcut")));
		macroNamePrompt.optionHelp
			.push_back(make_pair(Gettext::keyname(_("FKEY:Fn")),
					     _("Assign shortcut to"
					       " function key")));

		Macros::name macroName(0);

		{
			DefineMacroHelper macroHelper;

			macroNamePrompt=myServer::prompt(macroNamePrompt);

			if (macroHelper.defineFkeyFlag)
			{
				macroName=Macros::name(macroHelper.fkeyNum);
			}
			else
			{
				if (macroNamePrompt.abortflag)
					return true;

				const struct unicode_info *u=
					Gettext::defaultCharset();

				unicode_char *uc=
					(u->c2u)(u, ((string)macroNamePrompt)
						 .c_str(), NULL);

				size_t i;
				for (i=0; uc[i]; i++)
				{
					if ((unsigned char)uc[i] == uc[i] &&
					    strchr(" \t\r\n",
						   (unsigned char)uc[i]))
					{
						statusBar->clearstatus();
						statusBar->status(_("Macro name may not contain spaces."));
						statusBar->beepError();
						i=0;
						break;
					}
				}

				if (i == 0)
				{
					free(uc);
					return true;
				}

				try
				{
					vector<unicode_char> v(uc, uc+i);

					macroName=Macros::name(v);

					free(uc);
				} catch (...)
				{
					free(uc);
					LIBMAIL_THROW();
				}
			}
		}

		if (myServer::nextScreen)
			return true;

		size_t row1, pos1, row2, pos2;

		getMarkedRegion(row1, row2, pos1, pos2);

		if (row1 == row2 && pos1 == pos2)
			return true; // Shouldn't happen


		string cutText;

		size_t i;

		for (i=row1; i <= row2; i++)
		{
			vector<unicode_char> u;

			getText(i, u);

			if (i == row2)
				u.erase(u.begin() + pos2, u.end());

			if (i == row1)
				u.erase(u.begin(), u.begin() + pos1);

			if (i < row2)
				u.push_back('\n');
			u.push_back(0);

			char *p=unicode_toutf8(&u[0]);

			if (!p)
				outofmemory();
			try {
				cutText += p;

				free(p);
			} catch (...) {
				free(p);
			}
		}

		map<Macros::name, string>::iterator p=
			m->macroList.find(macroName);

		if (p != m->macroList.end())
			m->macroList.erase(p);

		m->macroList.insert(make_pair(macroName, cutText));

		macroDefined();
		statusBar->clearstatus();
		statusBar->status(_("Macro defined."));
		return true;
	}

	if (key.fkey())
	{
		Macros::name fk(key.fkeynum());

		Macros *mp=Macros::getRuntimeMacros();

		if (!mp)
			return true;

		map<Macros::name, std::string> &m=mp->macroList;

		map<Macros::name, string>::iterator p=
			m.find(fk);

		if (p != m.end())
			processMacroKey(0, p->second);
		return true;
	}

	if (key.plain())
	{
		if (key.key == '\x03')
			return false;

		CursesEditMessage::yankHelper::cut c;

		yank(key.key, true, c);
		return true;
	}

	return false;
}

void CursesEditMessage::macroDefined()
{
}

void CursesEditMessage::processMacroKey(size_t macroNameSize,
					std::string &repl_utf8)
{
	CursesEditMessage::yankHelper::macro mtext(repl_utf8);

	yank(CursesField::yankKey, true, mtext);
}

std::string CursesEditMessage::getConfigDir()
{
	return "";
}

void CursesEditMessage::extedited()
{
}

//////////////////////////////////////////////////////////////////////////
//
// Spell check replace prompt: suggestions, or manual choices.

class CursesEditMessage::ReplacePrompt : public CursesKeyHandler {

	vector<string> &wordlist;

	bool processKey(const Curses::Key &key);
	bool listKeys( vector< pair<string, string> > &list);

public:

	enum ReplaceAction {
		replaceCustom,
		replaceWord,
		replaceIgnore,
		replaceIgnoreAll,
		replaceAbort} replaceAction;

	string replaceWord_UTF8;

	ReplacePrompt( vector<string> &wordlistArg);
	~ReplacePrompt();

	ReplaceAction prompt();
};

bool CursesEditMessage::checkReplace(bool &doReplace, string &replaceWord,
				     SpellCheckerBase::Manager *manager)
{
	vector<string> suggestions;
	string errmsg;

	if (!manager->suggestions(replaceWord, suggestions, errmsg))
	{
		statusBar->clearstatus();
		statusBar->status(errmsg);
		statusBar->beepError();
		return false;
	}

	MONITOR(CursesEditMessage);

	for (;;)
	{
		ReplacePrompt::ReplaceAction action;
		string actionWord;

		{
			statusBar->status(_("Replace misspelled word with:"));
			ReplacePrompt prompt(suggestions);

			action=prompt.prompt();
			actionWord=prompt.replaceWord_UTF8;
		}

		if (myServer::nextScreen || DESTROYED())
			return false; // Something happened...

		Curses::keepgoing=true;

		switch (action) {
		case ReplacePrompt::replaceWord:
			replaceWord=actionWord;
			doReplace=true;
			return true;
		case ReplacePrompt::replaceIgnore:
			doReplace=false;
			return true;
		case ReplacePrompt::replaceIgnoreAll:
			if ((errmsg=manager->addToSession(replaceWord)).size()
			    > 0)
			{
				statusBar->clearstatus();
				statusBar->status(errmsg);
				statusBar->beepError();
				return false;
			}
			doReplace=false;
			return true;
		case ReplacePrompt::replaceAbort:
			return false;
		default:
			break;
		}

		myServer::promptInfo response=
			myServer
			::prompt(myServer
				 ::promptInfo(Gettext(_("Replace with: "))));

		if (response.abortflag ||
		    response.value.size() == 0)
			continue;

		replaceWord=response;

		char *p=unicode_convert(replaceWord.c_str(),
					Gettext::defaultCharset(),
					&unicode_UTF8);

		if (!p)
		{
			statusBar->clearstatus();
			statusBar->status(errmsg);
			statusBar->beepError();
			return false;
		}

		try {
			replaceWord=p;
			free(p);
		} catch (...) {
			free(p);
			LIBMAIL_THROW();
		}
		break;
	}

	doReplace=true;
	return true;
}

CursesEditMessage::ReplacePrompt::ReplacePrompt( vector<string> &wordlistArg)
	: CursesKeyHandler(PRI_STATUSHANDLER), wordlist(wordlistArg),
	  replaceAction(replaceCustom)
{
}

CursesEditMessage::ReplacePrompt::~ReplacePrompt()
{
}

bool CursesEditMessage::ReplacePrompt::listKeys( vector< pair<string, string> >
						 &list)
	// List of suggestions.
{
	string keyname[10];

	keyname[0]=Gettext::keyname(_("REPLACE0:0"));
	keyname[1]=Gettext::keyname(_("REPLACE1:1"));
	keyname[2]=Gettext::keyname(_("REPLACE2:2"));
	keyname[3]=Gettext::keyname(_("REPLACE3:3"));
	keyname[4]=Gettext::keyname(_("REPLACE4:4"));
	keyname[5]=Gettext::keyname(_("REPLACE5:5"));
	keyname[6]=Gettext::keyname(_("REPLACE6:6"));
	keyname[7]=Gettext::keyname(_("REPLACE7:7"));
	keyname[8]=Gettext::keyname(_("REPLACE8:8"));
	keyname[9]=Gettext::keyname(_("REPLACE9:9"));

	size_t i;

	for (i=0; i<10 && i<wordlist.size(); i++)
	{
		vector<wchar_t> wc;

		unicode_char *uc=(*unicode_UTF8.c2u)(&unicode_UTF8,
						     wordlist[i].c_str(),
						     NULL);

		if (!uc)
			continue;

		try {
			size_t j;

			for (j=0; uc[j]; j++)
				;

			wc.insert(wc.end(), uc, uc+j);
			free(uc);
		} catch (...) {
			free(uc);
			LIBMAIL_THROW();
		}

		wc.push_back(0);

		list.push_back(make_pair(keyname[i], Curses::wtomb(&wc[0])));
	}
	list.push_back(make_pair(Gettext::keyname(_("REPLACE:R")),
				 _("Replace")));
	list.push_back(make_pair(Gettext::keyname(_("IGNORE:I")),
				 _("Ignore")));
	list.push_back(make_pair(Gettext::keyname(_("IGNOREALL:A")),
				 _("Ignore All")));

	return true;
}

bool CursesEditMessage::ReplacePrompt::processKey(const Curses::Key &key)
{
	Gettext::Key *keys[10];

	keys[0]=&key_REPLACE0;
	keys[1]=&key_REPLACE1;
	keys[2]=&key_REPLACE2;
	keys[3]=&key_REPLACE3;
	keys[4]=&key_REPLACE4;
	keys[5]=&key_REPLACE5;
	keys[6]=&key_REPLACE6;
	keys[7]=&key_REPLACE7;
	keys[8]=&key_REPLACE8;
	keys[9]=&key_REPLACE9;

	size_t i;

	for (i=0; i<10 && i<wordlist.size(); i++)
	{
		if (key == *keys[i])
		{
			replaceAction=replaceWord;
			replaceWord_UTF8=wordlist[i];
			Curses::keepgoing=false;
			return true;
		}
	}

	if (key == key_REPLACE_K)
	{
		replaceAction=replaceCustom;
		Curses::keepgoing=false;
		return true;
	}

	if (key == key_IGNORE_K)
	{
		replaceAction=replaceIgnore;
		Curses::keepgoing=false;
		return true;
	}

	if (key == key_IGNOREALL_K)
	{
		replaceAction=replaceIgnoreAll;
		Curses::keepgoing=false;
		return true;
	}


	if (key == key_ABORT)
	{
		replaceAction=replaceAbort;
		Curses::keepgoing=false;
		return true;
	}

	statusBar->beepError();
	return true;
}

CursesEditMessage::ReplacePrompt::ReplaceAction
CursesEditMessage::ReplacePrompt::prompt()
{
	Curses::keepgoing=true;
	myServer::nextScreen=NULL;
	myServer::nextScreenArg=NULL;

	myServer::eventloop();

	if (myServer::nextScreen)
		return replaceAbort;

	return replaceAction;
}

//////////////////////////////////////////////////////////////////////////


vector<unicode_char>::iterator
CursesEditMessage::getIndexOfCol(vector<unicode_char> &line,
				 size_t colNum)
{
	vector<size_t> pos;

	getTextPos(line, pos);

	return getIndexOfCol(line, pos, colNum);
}

vector<unicode_char>::iterator
CursesEditMessage::getIndexOfCol(vector<unicode_char> &line,
				 vector<size_t> &pos,
				 size_t colNum)
{
	size_t n=pos.size()-1;

	while (n > 0)
	{
		if (pos[n] <= colNum)
			break;
		--n;
	}

	return line.begin() + n;

#if 0
	vector<unicode_char>::iterator indexp=line.begin();

	vector<unicode_char>::iterator b=line.begin(), e=line.end();

	size_t col=0;

	while (b != e)
	{
		if (col <= colNum)
			indexp=b;

		if (*b == '\t')
		{
			col += WRAPTABSIZE;
			col -= (col % WRAPTABSIZE);
		}
		else
			col++;
		b++;
	}
	if (col <= colNum)
		indexp=e;

	return indexp;
#endif
}

void CursesEditMessage::mark()
{
	vector<unicode_char> line;

	getText(cursorRow, line);

	markRow=cursorRow;
	markCursorPos= getIndexOfCol(line, cursorCol) - line.begin();
	marked=true;
}

void CursesEditMessage::end()
{
	vector<unicode_char> line;
	vector<size_t> positions;

	getText(cursorRow, line);
	getTextPos(line, positions);

	cursorCol=positions.end()[-1];

	drawLine(cursorRow);
}

void CursesEditMessage::yank(wchar_t k, bool doUpdate,
			     CursesEditMessage::yankHelper &yh)
{
	if ( k >= 0 && k < ' ' && k != '\t' &&
	     k != CursesField::yankKey)
		return;

	modified();

	vector<unicode_char> line;
	vector<size_t> metrics;

	getText(cursorRow, line);
	getTextPos(line, metrics);

	vector<unicode_char>::iterator cursorPos=
		getIndexOfCol(line, cursorCol);

	if (k == CursesField::yankKey)
	{
		// Temporarily split the line at the paste point

		vector<unicode_char> lineSplit;

		lineSplit.insert(lineSplit.end(), cursorPos,
				 line.end());

		text_UTF8.insert(text_UTF8.begin() + cursorRow + 1, "");

		setText(cursorRow + 1, lineSplit);
		line.erase(cursorPos, line.end());

		setText(cursorRow, line);

		size_t newCursorRow=cursorRow+1;

		string insertBuffer=yh;

		bool oneMoreLine;

		do
		{
			size_t p=insertBuffer.find('\n');

			string l;

			oneMoreLine=false;

			if (p == insertBuffer.npos)
			{
				l=insertBuffer;
				insertBuffer="";
			}
			else
			{
				oneMoreLine=true;
				l=insertBuffer.substr(0, p);
				insertBuffer=insertBuffer.substr(p+1);
			}

			text_UTF8.insert(text_UTF8.begin() + newCursorRow, l);
			++newCursorRow;
		} while (oneMoreLine);

		// Now merge the splits

		text_UTF8[cursorRow] += text_UTF8[cursorRow+1];

		text_UTF8.erase(text_UTF8.begin() + cursorRow+1);
		--newCursorRow; // Account for this erase.

		--newCursorRow;

		getText(newCursorRow, line);

		vector<size_t> tempPos;

		getTextPos(line, tempPos);

		cursorCol=tempPos.end()[-1];

		text_UTF8[newCursorRow] += text_UTF8[newCursorRow+1];
		text_UTF8.erase(text_UTF8.begin() + newCursorRow+1);
		cursorRow=newCursorRow;
		marked=false;
		if (doUpdate)
			draw();
	}
	else
	{
		cursorCol=metrics[cursorPos - line.begin()];

		line.insert(cursorPos, k);
		setText(cursorRow, line);
		right();

		size_t cursorPos=getIndexOfCol(line, cursorCol) - line.begin();

		if (CursesMoronize::enabled)
		{
			size_t n=cursorPos > 4 ? 4:cursorPos;
			char buf[5];
			size_t i;

			for (i=0; i<n; i++)
			{
				unicode_char c=line[cursorPos-1-i];

				if ((unsigned char)c != c)
					break;
				buf[i]= (char)c;
			}
			if (i > 0)
			{
				wchar_t repl_c;

				buf[i]=0;
				i=CursesMoronize::moronize(buf, repl_c);

				if (i > 0)
				{
					while (i)
					{
						CursesEditMessage::
							processKeyInFocus(Curses::Key(Curses::Key::BACKSPACE));
						--i;
					}
					CursesEditMessage::
						processKeyInFocus(Curses::Key(repl_c));
					return;
				}
			}
		}

		Macros *mp=Macros::getRuntimeMacros();

		if (mp)
		{
			map<Macros::name, string> &macroMap=mp->macroList;

			map<Macros::name, string>::iterator b=macroMap.begin(),
				e=macroMap.end();

			for ( ; b != e; ++b)
			{
				if (b->first.f)
					continue;
				size_t s=b->first.n.size();

				if (s > cursorPos ||
				    !equal(b->first.n.begin(),
					   b->first.n.end(),
					   line.begin()+cursorPos-s))
					continue;

				line.erase(line.begin()+cursorPos-s,
					   line.begin()+cursorPos);
				setText(cursorRow, line);
				cursorCol=metrics[cursorPos-s];
				processMacroKey(0, b->second);
				return;
			}
		}

		inserted(cursorRow, line);
	}
}

bool CursesEditMessage::search(bool doUpdate, bool doWrap,
			       mail::Search &searchEngine,
			       size_t searchStrSize)
{
	searchEngine.reset();

	// We want to keep track of the current cursor position before
	// matching each unicode char.  When a match is found, we can back-
	// track and know where the search string started originally.

	vector< pair<size_t, size_t> > matchpos;

	matchpos.insert(matchpos.end(), searchStrSize, make_pair(0, 0));

	vector<unicode_char> line;

	getText(cursorRow, line);

	size_t cursorPos=getIndexOfCol(line, cursorCol) - line.begin();

	size_t searchRow=cursorRow;
	size_t searchPos=cursorPos;

	vector<unicode_char> lineuc;

	// We begin the search in the middle of the line, so load a partial
	// line into lineuc, but make sure the cursor position doesn't change.

	lineuc.insert(lineuc.end(), cursorPos, ' ');
	lineuc.insert(lineuc.end(), line.begin() + cursorPos, line.end());

	bool needSpace=false;
	// A space in the search string matches any amount of whitespace
	// in the text.  Therefore, whitespace in the text sets a flag,
	// if the flag is set before matching a non-whitespace text char,
	// a single space gets fed to the search engine.

	size_t matchIndex=0;

	size_t rowNum=text_UTF8.size()+1;
	// Make sure the whole text buffer gets searched

	bool wrappedAround=false;

	while ( !searchEngine )
	{
		if (searchPos >= lineuc.size())
		{
			// Simulated whitespace

			if (!needSpace)
			{
				matchpos[matchIndex]=
					make_pair(searchRow, searchPos);
				matchIndex=(matchIndex+1) % matchpos.size();
				needSpace=true;
			}
			if (++searchRow >= numLines())
			{
				wrappedAround=true;
				searchRow=0;
				if (!doWrap)
					return false;
			}

			lineuc.clear();

			unicode_char *uc=
				(*unicode_UTF8.c2u)(&unicode_UTF8,
						    text_UTF8[searchRow]
						    .c_str(), NULL);

			if (!uc)
				outofmemory();

			try {
				size_t i;

				for (i=0; uc[i]; i++)
					;

				lineuc.insert(lineuc.end(), uc, uc+i);

				free(uc);
			} catch (...) {
				free(uc);
				LIBMAIL_THROW();
			}

			searchPos=0;

			if (--rowNum == 0)
			{
				statusBar->clearstatus();
				statusBar->status(_("Not found"));
				return false;
			}
		}
		else if ( (unsigned char)lineuc[searchPos] == lineuc[searchPos]
			  && isspace(lineuc[searchPos]))
		{
			if (!needSpace)
			{
				matchpos[matchIndex]=
					make_pair(searchRow, searchPos);
				matchIndex=(matchIndex+1) % matchpos.size();
				needSpace=true;
			}
			++searchPos;
		}
		else
		{
			unicode_char ucBuf[2];

			char utf8buf[UNICODE_UTF8_MAXLEN];
			size_t l;

			if (needSpace)
			{
				searchEngine << ' ';
				needSpace=false;

				if (!!searchEngine)
					break;
			}

			ucBuf[0]=unicode_uc(lineuc[searchPos]);
			ucBuf[1]=0;

			l=unicode_utf8_fromu_pass(ucBuf, utf8buf);

			matchpos[matchIndex]=
				make_pair(searchRow, searchPos);
			matchIndex=(matchIndex+1) % matchpos.size();

			size_t i;

			for (i=0; i<l; i++)
				searchEngine << utf8buf[i];

			++searchPos;
		}
	}

	// Found the search string, select it

	if (wrappedAround)
	{
		statusBar->clearstatus();
		statusBar->status(_("Wrapped around from the beginning"));
	}

	marked=true;
	markRow=matchpos[matchIndex].first;
	markCursorPos=matchpos[matchIndex].second;

	cursorRow=searchRow;

	getText(cursorRow, line);

	vector<size_t> metrics;
	getTextPos(line, metrics);

	cursorCol=metrics[searchPos];

	int dummy1, dummy2;
	getCursorPosition(dummy1, dummy2); // Make sure on screen
	if (doUpdate)
		draw();
	return true;
}


bool CursesEditMessage::getMarkedRegion(size_t &row1, size_t &row2,
					size_t &pos1, size_t &pos2)
{
	bool swapped=false;

	row1=markRow;
	pos1=markCursorPos;

	vector<unicode_char> line;

	getText(cursorRow, line);

	row2=cursorRow;
	pos2=getIndexOfCol(line, cursorCol) - line.begin();

	if (row2 < row1 || (row2 == row1 && pos2 < pos1))
	{
		row1 ^= row2 ^= row1 ^= row2;
		pos1 ^= pos2 ^= pos1 ^= pos2;
		swapped=true;
	}

	return swapped;
}

void CursesEditMessage::deleteChar()
{
	modified();

	if (marked)
	{
		size_t row1, pos1, row2, pos2;

		getMarkedRegion(row1, row2, pos1, pos2);

		if (row1 != row2 || pos1 != pos2)
		{
			erase();

			CursesField::cutBuffer="";

			size_t i;

			vector<unicode_char> u;

			const struct unicode_info *uc=
				Gettext::defaultCharset();

			for (i=row1; i <= row2; i++)
			{
				getText(i, u);

				if (i == row2)
					u.erase(u.begin() + pos2, u.end());

				if (i == row1)
					u.erase(u.begin(), u.begin() + pos1);

				if (i < row2)
					u.push_back('\n');
				u.push_back(0);

				char *p=unicode_fromCharset(uc, &u[0],
							    currentEntityAltList);

				if (!p)
					outofmemory();
				try {
					CursesField::cutBuffer += p;

					free(p);
				} catch (...) {
					free(p);
				}
			}

			vector<unicode_char> line1, line2;

			getText(row1, line1);
			getText(row2, line2);

			line1.erase(line1.begin() + pos1, line1.end());
			line1.insert(line1.end(),
				     line2.begin() + pos2, line2.end());

			text_UTF8.erase(text_UTF8.begin() + row1,
					text_UTF8.begin() + row2 + 1);
			text_UTF8.insert(text_UTF8.begin() + row1, "");

			setText(row1, line1);

			vector<size_t> metrics;
			getTextPos(line1, metrics);

			cursorRow=row1;
			cursorCol=metrics[pos1];

			marked=false;
			draw();
			int dummy1, dummy2;
			getCursorPosition(dummy1, dummy2);

			return;
		}
		marked=false;
	}

	vector<unicode_char> uc;
	vector<size_t> metrics;

	getText(cursorRow, uc);
	getTextPos(uc, metrics);

	size_t cursorPos=getIndexOfCol(uc, cursorCol) - uc.begin();

	if (cursorPos >= uc.size())
	{
		if (cursorRow + 1 < text_UTF8.size())
		{
			erase();

			text_UTF8[cursorRow] += text_UTF8[cursorRow+1];
			text_UTF8.erase(text_UTF8.begin() + cursorRow+1);

			cursorCol=metrics[cursorPos];
			draw();
		}
	}
	else
	{
		cursorCol=metrics[cursorPos];

		uc.erase(uc.begin() + cursorPos);

		setText(cursorRow, uc);
		drawLine(cursorRow);
	}
}

// Inserted text, see if the line needs to be wrapped.

void CursesEditMessage::inserted(size_t row, vector<unicode_char> &line)
{
	bool needDraw=false; // redraw everything.

	for (;;)
	{
		size_t cursorPosFromEnd=0;
		bool cursorWrapped=false;

		size_t wrappedLineSize;
		// Size of the original line, after wrapping.

		{
			vector< vector<unicode_char> > wrappedLines;

			{
				WrapText wrapped(line, LINEW);

				wrappedLines=wrapped;

				if (wrappedLines.size() > 1 &&
				    wrappedLines.end()[-1].size() == 0)
				{
					// See last comment in Curses::wordWrap
					// (we need to undo this

					wrappedLines
						.erase(wrappedLines.end()-1);
					wrappedLines.end()[-1].push_back(' ');
				}

				if (wrappedLines.size() < 2)
				{
					setText(row, line);
					break; // Nothing needs to be wrapped
				}

			}

			wrappedLineSize=wrappedLines[0].size();

			wrappedLines[0].push_back(' '); // rfc 2646
			setText(row, wrappedLines[0]);

			// Move the cursor to the next line, if necessary.

			if (cursorRow == row)
			{
				cursorPosFromEnd=line.end() -
					getIndexOfCol(line, cursorCol);

				if (line.size() - cursorPosFromEnd
				    > wrappedLineSize)
				{
					++cursorRow;
					cursorWrapped=true;
				}
			}

			// Take the remaining lines, and rebuild them

			line.clear();

			vector< vector<unicode_char> >::iterator
				b=wrappedLines.begin(),
				e=wrappedLines.end();

			while (++b != e)
			{
				if (line.size() > 0 &&
				    line.end()[-1] != ' ')
					line.push_back(' ');
				line.insert(line.end(), b->begin(), b->end());
			}
		}

		if (cursorWrapped)
		{
			size_t cursorPos =
				cursorPosFromEnd < line.size()
				? line.size() - cursorPosFromEnd:0;

			vector<size_t> metrics;

			getTextPos(line, metrics);

			cursorCol=metrics[cursorPos];
		}
			
		needDraw=1;

		++row;

		bool needInsert=true;

		if (row < text_UTF8.size())
		{
			vector<unicode_char> nextline;

			getText(row, nextline);

			if (nextline.size() > 0 &&
			    nextline[0] != '>')
			{
				if (line.size() > 0 &&
				    line.end()[-1] != ' ')
					line.push_back(' ');
				line.insert(line.end(),
					    nextline.begin(),
					    nextline.end());
				needInsert=false;
			}
		}

		if (needInsert)
		{
			// overflow doesn't fit.  Insert.

			text_UTF8.insert(text_UTF8.begin() + row, "");
		}
	}

	if (needDraw)
		draw();
	else
		drawLine(row);
}

void CursesEditMessage::left(bool moveOff)
{
	vector<unicode_char> line;
	vector<size_t> metrics;

	getText(cursorRow, line);
	getTextPos(line, metrics);

	size_t cursorPos=getIndexOfCol(line, cursorCol) - line.begin();

	if (cursorPos == 0)
	{
		if (cursorRow == 0)
		{
			if (moveOff)
				transferPrevFocus();
		}
		else
		{
			--cursorRow;

			getText(cursorRow, line);
			getTextPos(line, metrics);

			cursorCol=metrics.end()[-1];
			drawLine(cursorRow+1);
		}
	}
	else cursorCol=metrics[cursorPos-1];

	drawLine(cursorRow);
}

void CursesEditMessage::right()
{
	vector<unicode_char> line;
	vector<size_t> metrics;

	getText(cursorRow, line);
	getTextPos(line, metrics);

	size_t cursorPos=getIndexOfCol(line, cursorCol) - line.begin();


	if (cursorPos >= line.size())
	{
		if (cursorRow + 1 < text_UTF8.size())
		{
			cursorCol=0;
			cursorLineHorizShift=0;
			++cursorRow;
			drawLine(cursorRow - 1);
		}
	}
	else
	{
		cursorCol=metrics[cursorPos+1];
	}
	drawLine(cursorRow);
}


int CursesEditMessage::getCursorPosition(int &row, int &col)
{
	vector<unicode_char> uc_expanded;

	getText(cursorRow, uc_expanded);

	size_t w=getWidth();

	vector<size_t> metrics;

	getTextPos(uc_expanded, metrics);

	size_t p=metrics[getIndexOfCol(uc_expanded, metrics, cursorCol)
			 - uc_expanded.begin()];

	expandTabs(uc_expanded, metrics);

	if (uc_expanded.size() < w)
	{
		if (cursorLineHorizShift > 0)
		{
			cursorLineHorizShift=0;
			drawLine(cursorRow);
		}
	}
	else
		if (p <= cursorLineHorizShift
		    && cursorLineHorizShift > 0)
		{
			cursorLineHorizShift=p-1;
			drawLine(cursorRow);
		}
		else
		{
			size_t ww=w;

			if (ww > 0)
				--ww;

			if (cursorLineHorizShift + ww <= p)
			{
				cursorLineHorizShift= p-ww+1;
				drawLine(cursorRow);
			}
		}

	row=cursorRow;

	if (cursorLineHorizShift < (size_t)p)
		p -= cursorLineHorizShift;
	else
		p=0;

	col=p;
	Curses::getCursorPosition(row, col);

	if (marked)
	{
		size_t row1, col1, row2, col2;

		if (getMarkedRegion(row1, row2, col1, col2))
			return 0;
	}

	return 1;
}

void CursesEditMessage::draw()
{
	size_t firstRow, nrows;

	getVerticalViewport(firstRow, nrows);

	size_t i;

	for (i=0; i<nrows; i++)
		drawLine(firstRow + i);
}

void CursesEditMessage::erase()
{
	size_t firstRow, nrows;

	getVerticalViewport(firstRow, nrows);

	size_t i;

	vector<wchar_t> spaces;

	spaces.insert(spaces.begin(), getWidth(), ' ');
	spaces.push_back(0);

	for (i=0; i<nrows; i++)
		writeText(&*spaces.begin(), i+firstRow, 0, CursesAttr());
}

void CursesEditMessage::eraseAllText()
{
	text_UTF8.clear();
	text_UTF8.push_back(string(""));
	cursorRow=0;
	cursorCol=0;
	cursorLineHorizShift=0;
	marked=false;
	draw();
}

// Draw the indicated line.

void CursesEditMessage::drawLine(size_t lineNum)
{
	vector<unicode_char> chars;
	vector<size_t> pos;

	getText(lineNum, chars);
	getTextPos(chars, pos);

	size_t w=getWidth();

	size_t col;

	size_t selectedFirst=0; // First char to highlight
	size_t selectedLast=0;  // First char to highlight no more

	if (marked)
	{
		size_t row1, col1;
		size_t row2, col2;

		getMarkedRegion(row1, row2, col1, col2);

		if (row1 <= lineNum && lineNum <= row2)
		{
			if (row1 == lineNum)
				selectedFirst=col1;

			if (row2 == lineNum)
				selectedLast=col2;
			else
				selectedLast=chars.size();
		}
	}

	selectedFirst=pos[selectedFirst];
	selectedLast=pos[selectedLast];

	expandTabs(chars, pos);

	col=0;
	if (lineNum == cursorRow && cursorLineHorizShift > 0

	    /*
	    ** If cursor leaves the edit area we don't want to leave the
	    ** text display shifted, UNLESS the cursor is on the status line.
	    */

	    && (hasFocus() || statusBar->prompting()))
	{
		size_t n=cursorLineHorizShift + 1; // 1st char is <

		if (n < chars.size())
			chars.erase(chars.begin(), chars.begin() + n);
		else
			chars.clear();

		if (selectedFirst >= n)
			selectedFirst -= n;
		else
			selectedFirst=0;

		if (selectedLast >= n)
			selectedLast -= n;
		else
			selectedLast=0;

		if (w > 0)
			--w;

		col=1;

		writeText(larr, lineNum, 0,
			  CursesAttr().setReverse());
	}

	size_t lw=chars.size();

	if (lw < w)
		chars.insert(chars.end(), w - lw, ' ');

	if (w <= 0)
		return;

	if (chars.size() > w)
	{
		chars.erase(chars.begin() + w - 1, chars.end());

		CursesAttr attr;

		attr.setReverse();
		writeText(rarr, lineNum, col+w-1, attr);
	}

	if (selectedFirst >= chars.size())
		selectedFirst=selectedLast=0;

	CursesAttr attr;

	if (selectedFirst > 0)
	{
		vector<wchar_t> before;

		before.insert(before.end(), chars.begin(),
			      chars.begin() + selectedFirst);
		before.push_back(0);

		writeText(&*before.begin(), lineNum, col, attr);
	}

	attr.setReverse();

	if (selectedLast > selectedFirst)
	{
		vector<wchar_t> selected;

		selected.insert(selected.end(), chars.begin() + selectedFirst,
				chars.begin() + selectedLast);
		selected.push_back(0);

		writeText(&*selected.begin(), lineNum,
			  col + selectedFirst, attr);
	}

	attr.setReverse(false);

	if (selectedLast < chars.size())
	{
		vector<wchar_t> after;

		after.insert(after.end(), chars.begin() + selectedLast,
			     chars.end());
		after.push_back(0);

		writeText(&*after.begin(), lineNum,
			  col + selectedLast, attr);
	}
}

void CursesEditMessage::load(istream &i)
{
	string line;

	size_t n=0;

	const struct unicode_info *uc=Gettext::defaultCharset();

	while (!i.eof())
	{
		line="";

		if (getline(i, line).fail() && !i.eof())
		{
			statusBar->clearstatus();
			statusBar->status(strerror(errno));
			statusBar->beepError();
			return;
		}

		if (line.size() == 0 && i.eof())
			continue;


		char *p=unicode_ctoutf8( uc, line.c_str(), NULL);

		if (!p)
			outofmemory();

		try {
			if (n < text_UTF8.size())
				text_UTF8[n]=p;
			else
				text_UTF8.push_back(p);
			free(p);
		} catch (...) {
			free(p);
			LIBMAIL_THROW();
		}
		++n;
	}
}

void CursesEditMessage::save(ostream &o)
{
	const struct unicode_info *uc=Gettext::defaultCharset();

	std::vector<std::string>::iterator b, e;

	b=text_UTF8.begin();
	e=text_UTF8.end();

	while (b != e)
	{
		string s= *b++;

		char *p=unicode_cfromutf8(uc, s.c_str(), NULL);

		if (!p)
			outofmemory();

		o << p << endl;
		free(p);
	}
}

bool CursesEditMessage::processKey(const Curses::Key &key)
{
	return false;
}

bool CursesEditMessage::listKeys( vector< pair<string, string> > &list)
{
	list.push_back(make_pair(Gettext::keyname(_("MARK_K:^ ")),
				 _("Mark")));
	list.push_back(make_pair(Gettext::keyname(_("JUSTIFY_K:^J")),
				 _("Justify")));
	list.push_back(make_pair(Gettext::keyname(_("CLREOL_K:^K")),
				 _("Line Clear")));
	list.push_back(make_pair(Gettext::keyname(_("SEARCH_K:^S")),
				 _("Search")));
	list.push_back(make_pair(Gettext::keyname(_("REPLACE_K:^R")),
				 _("Srch/Rplce")));
	list.push_back(make_pair(Gettext::keyname(_("CUT_K:^W")),
				 _("Cut")));
	list.push_back(make_pair(Gettext::keyname(_("YANK_K:^Y")),
				 _("Paste")));
	list.push_back(make_pair(Gettext::keyname(_("INSERT_K:^G")),
				 _("Insert File")));
	list.push_back(make_pair(Gettext::keyname(_("DICTSPELL_K:^D")),
				 _("Dict Spell")));
	list.push_back(make_pair(Gettext::keyname(_("CANCEL_K:^C")),
				 _("Cancel/Exit")));
	list.push_back(make_pair(Gettext::keyname(_("MACRO_K:^N")),
				 _("New Macro")));
	if (externalEditor.size() > 0)
		list.push_back(make_pair(Gettext::keyname(_("EDITOR_K:^U")),
					 _("Ext Editor")));
	return false;
}

#if 1
void CursesEditMessage::getTextPos(std::vector<unicode_char> &line,
				   std::vector<size_t> &pos)
{
	vector<wchar_t> wc;
	wc.insert(wc.end(), line.begin(), line.end());

	return (Curses::getTextPos(wc, pos));
}


void CursesEditMessage::expandTabs(std::vector<unicode_char> &line,
				   std::vector<size_t> &pos)
{
	vector<wchar_t> wc;
	wc.insert(wc.end(), line.begin(), line.end());

	Curses::expandTabs(wc, pos);

	line.clear();
	line.insert(line.begin(), wc.begin(), wc.end());
}

#endif
