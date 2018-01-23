/* $Id: cursesstatusbar.C,v 1.5 2003/10/12 01:36:33 mrsam Exp $
**
** Copyright 2002, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "curses_config.h"
#include "cursesstatusbar.H"
#include "cursesscreen.H"
#include "cursescontainer.H"

#include <algorithm>

using namespace std;

string CursesStatusBar::extendedErrorPrompt;
string CursesStatusBar::shortcut_next_key;
string CursesStatusBar::shortcut_next_descr;
wchar_t CursesStatusBar::shortcut_next_keycode= 'O' & 31;

CursesStatusBar::CursesStatusBar(CursesScreen *parent) :
	CursesContainer(parent), CursesKeyHandler(PRI_STATUSHANDLER),
	busyCounter(0), parentScreen(parent), fieldActive(NULL),
	currentShortcutPage(0)
{
	clearStatusTimer=this;
	clearStatusTimer= &CursesStatusBar::clearstatus;
	resetRow();
}

// Reposition the status bar after a wrapped text message appeared or
// disappered.

void CursesStatusBar::resetRow()
{
	int h=parentScreen->getHeight();
	int hh=getHeight();

	if (h > hh)
	{
		h -= hh;
		setRow(h);
	}
	else
		setRow(2);
}

CursesStatusBar::~CursesStatusBar()
{
	if (fieldActive)
	{
		delete fieldActive;
		CursesKeyHandler::handlerListModified=true;
		// Rebuild shortcuts
	}
}

int CursesStatusBar::getWidth() const
{
	return getParent()->getWidth();
}

// The height is usually 3 rows, but may be larger when a wrapped message
// is shown

int CursesStatusBar::getHeight() const
{
	int r=3;

	if (extendedErrorMsg.size() > 0)
		r += extendedErrorMsg.size() + 1;

	return r;
}

void CursesStatusBar::resized()
{
	resetRow();
	CursesKeyHandler::handlerListModified=true; // Rebuild shortcuts
	draw();
}

bool CursesStatusBar::progressWanted()
{
	if (extendedErrorMsg.size() > 0)
		return false;

	time_t now=time(NULL);

	if (progressText.size() > 0 && progressTime == now)
		return false; // Too soon

	progressTime=now;
	return true;
}

void CursesStatusBar::progress(string progress)
{
	progressText.clear();
	mbtow(progress.c_str(), progressText);
	draw();
	flush();
}

void CursesStatusBar::setStatusBarAttr(CursesAttr a)
{
	attrStatusBar=a;
	draw();
}

void CursesStatusBar::setHotKeyAttr(CursesAttr a)
{
	attrHotKey=a;
	draw();
}

void CursesStatusBar::setHotKeyDescr(CursesAttr a)
{
	attrHotKeyDescr=a;
	draw();
}


void CursesStatusBar::draw()
{
	string spaces="";

	// To save some time, call rebuildShortcuts() only when necessary

	if (CursesKeyHandler::handlerListModified)
		rebuildShortcuts();

	static const char throbber[]={'|', '/', '-', '\\'};

	size_t w=getWidth();

	vector<wchar_t> statusLine=statusText;

	statusLine.insert(statusLine.begin(), ' ');

	if (busyCounter)
	{
		if (busyCounter > (int)sizeof(throbber))
			busyCounter=1;
		statusLine.insert(statusLine.begin(),
				  (wchar_t)throbber[busyCounter-1]);
	}
	else
		statusLine.insert(statusLine.begin(),
				  ' ');

	if (extendedErrorMsg.size() == 0)
		statusLine.insert(statusLine.end(),
				  progressText.begin(),
				  progressText.end());

	size_t statusline_w;

	// Chop status line that's too big.

	for (;;)
	{
		statusLine.push_back(0);
		statusline_w=getTextLength(&statusLine[0]);
		statusLine.erase(statusLine.end()-1);

		if (statusline_w <= w || statusLine.size() == 0)
			break;
		statusLine.erase(statusLine.end()-1);
	}

	if (statusline_w < w)
		statusLine.insert(statusLine.end(), w-statusline_w, ' ');

	CursesAttr attr=attrStatusBar;

	attr.setReverse();
	statusLine.push_back(0);
	writeText(&statusLine[0], getHeight()-3, 0, attr);

	if (extendedErrorMsg.size() > 0)
	{
		statusLine.clear();
		statusLine.insert(statusLine.begin(), w, ' ');
		statusLine.push_back(0);
		writeText(&statusLine[0], 0, 0, attr);
		size_t n;

		for (n=0; n<extendedErrorMsg.size(); n++)
		{
			writeText(&extendedErrorMsg[n][0],
				  n+1, 0, attrStatusBar);
		}
	}

	// Clear the shortcut area.

	statusLine.clear();
	statusLine.insert(statusLine.begin(), w, ' ');
	statusLine.push_back(0);

	int r=1;

	if (extendedErrorMsg.size() > 0)
		r += extendedErrorMsg.size()+1;

	writeText(&statusLine[0], r, 0, CursesAttr());
	writeText(&statusLine[0], r+1, 0, CursesAttr());

	// Draw the current shortcut page.

	if (extendedErrorMsg.size() == 0 &&
	    currentShortcutPage < shortcuts.size())
	{

		vector <vector <
			pair< vector<wchar_t>, vector<wchar_t> > > >
			&shortcutPage= shortcuts[currentShortcutPage];

		size_t i;

		for (i=0; i < shortcutPage.size(); i++)
		{
			vector< pair< vector<wchar_t>, vector<wchar_t> > >
				&column=shortcutPage[i];

			size_t j;

			for (j=0; j<column.size(); j++)
			{
				pair< vector<wchar_t>, vector<wchar_t> >
					&row=column[j];

				vector<wchar_t> cpy=row.first;

				if (shortcuts.size() > 1 ||
				    row.first.size() > 1 ||
				    row.second.size() > 1)
					while (cpy.size() < max_sc_nlen)
						cpy.insert(cpy.begin(), ' ');

				CursesAttr h=attrHotKey;

				writeText( &cpy[0], j+1,
					   i * (max_sc_nlen + max_sc_dlen + 1),
					   h.setReverse());

				CursesAttr a;

				wchar_t *s=&row.second[0];

				if (*s == '/' && s[1] >= '0' && s[1] <= '9')
				{
					a.setBgColor(s[1] - '0');
					s += 2;
				}
				else
				{
					a=attrHotKeyDescr;
				}

				writeText( s, j+1,
					   i * (max_sc_nlen + max_sc_dlen + 1)
					   + max_sc_nlen, a);
			}
		}
	}
	CursesContainer::draw();
}

class CursesStatusBarShortcutSort {
public:
	bool operator()(pair<string, string> a,
			pair<string, string> b)
	{
		return (strcoll( a.first.c_str(), b.first.c_str()) < 0);
	}
};

//
// Create a sorted key shortcut list from all installed keyhandlers.
//

static void createShortcuts(size_t &max_sc_nlen, size_t &max_sc_dlen,
			    vector< pair< vector<wchar_t>,
			    vector<wchar_t> > > &keys_w)
{
	vector< pair<string, string> > keys;

	list<CursesKeyHandler *>::const_iterator
		kb=CursesKeyHandler::begin(),
		ke=CursesKeyHandler::end();

	while (kb != ke)
		if ((*kb++)->listKeys(keys))
			break;

	sort(keys.begin(), keys.end(),
	     CursesStatusBarShortcutSort());

	vector< pair<string, string> >::iterator b=keys.begin(), e=keys.end();

	while (b != e)
	{
		vector<wchar_t> first_w;
		vector<wchar_t> second_w;

		Curses::mbtow(b->first.c_str(), first_w);
		Curses::mbtow(b->second.c_str(), second_w);

		b++;

		first_w.push_back(0);
		second_w.push_back(0);

		if (first_w.size() > max_sc_nlen)
			max_sc_nlen=first_w.size();

		size_t ss=second_w.size();

		if (ss > 2 && second_w[0] == '/')
			ss -= 2;

		if (ss > max_sc_dlen)
			max_sc_dlen=ss;

		keys_w.push_back(make_pair(first_w, second_w));
	}
}

void CursesStatusBar::rebuildShortcuts()
{
	CursesKeyHandler::handlerListModified=false;

	currentShortcutPage=0;

	vector<wchar_t> nextpage_n;
	vector<wchar_t> nextpage_d;

	if (shortcut_next_key.size() == 0)
		shortcut_next_key="^O";

	if (shortcut_next_descr.size() == 0)
		shortcut_next_descr="...mOre";

	mbtow(shortcut_next_key.c_str(), nextpage_n);
	mbtow(shortcut_next_descr.c_str(), nextpage_d);

	nextpage_n.push_back(0);
	nextpage_d.push_back(0);

	max_sc_nlen=nextpage_n.size();
	max_sc_dlen=nextpage_d.size();

	vector< pair< vector<wchar_t>, vector<wchar_t> > > keys_w;

	createShortcuts(max_sc_nlen, max_sc_dlen, keys_w);

	// Does everything fit on one page?

	size_t w=getWidth();

	size_t ncols=w / (max_sc_nlen + 1 + max_sc_dlen);

	if (ncols <= 0)
		ncols=1;

	shortcuts.clear();
	bool multiplePages=false;

	size_t n=keys_w.size();
	size_t i=0;

	while (i < n)
	{
		if (n - i > ncols * 2)
			multiplePages=true;

		vector < vector < pair < vector<wchar_t>, vector<wchar_t> > > >
			columns;

		size_t j;

		for (j=0; j < ncols; j++ )
		{
			vector < pair < vector<wchar_t>, vector<wchar_t> > >
				column;

			if (i < n)
			{
				column.push_back( keys_w[i++] );
			}
			else
			{
				vector<wchar_t> zero;

				zero.push_back(0);

				column.push_back (make_pair(zero, zero));
			}

			// Add ^O to last cell on each page.

			if (j + 1 == ncols && multiplePages)
			{
				pair< vector<wchar_t>, vector<wchar_t> >
					p=make_pair(nextpage_n,
						    nextpage_d);
				column.push_back(p);
			}
			else if (i < n)
			{
				column.push_back( keys_w[i++] );
			}
			else
			{
				vector<wchar_t> zero;

				zero.push_back(0);

				column.push_back (make_pair(zero, zero));
			}

			columns.push_back(column);
		}

		shortcuts.push_back(columns);
	}
}

void CursesStatusBar::status(string text, statusLevel level)
{
	if ((statusText.size() > 0 || extendedErrorMsg.size() > 0) &&
	    currentLevel > level || fieldActive != NULL)
	{
		// Message to low of a priority

		if (CursesKeyHandler::handlerListModified || busyCounter)
		{
			busyCounter=0;
			draw(); // Update the list of shortcuts
		}
		return;
	}

	busyCounter=0;

	statusText.clear();
	progressText.clear();

	string::iterator b=text.begin(), e=text.end(), start;

	while (b != e)
	{
		start=b;

		while (b != e)
		{
			if (*b == '\n')
				break;
			b++;
		}

		vector<wchar_t> statusLine;
		string l=string(start, b);

		mbtow(l.c_str(), statusLine);

		statusText.insert(statusText.end(),
				  statusLine.begin(),
				  statusLine.end());

		if (b != e)
		{
			b++;
			statusText.push_back('\n');
		}
	}

	currentLevel=level;

	size_t w=getWidth();

	if (text.find('\n') == text.npos &&
	    (size_t)(getTextLength(text.c_str()) + 2) < w)
	{
		if (extendedErrorMsg.size() > 0)
		{
			extendedErrorMsg.clear();
			parentScreen->resized();
		}
	}
	else
	{
		// Error message too long - wrap it.

		extendedErrorMsg.clear();

		if (extendedErrorPrompt.size() > 0)
		{
			statusText.push_back('\n');
			statusText.push_back('\n');

			vector<wchar_t> statusLine;

			mbtow(extendedErrorPrompt.c_str(), statusLine);

			statusText.insert(statusText.end(),
					  statusLine.begin(),
					  statusLine.end());

		}

		vector<wchar_t>::iterator b=statusText.begin(),
			e=statusText.end(), c, d;

		while (b != e)
		{
			c=b;
			d=statusText.end();

			while (b != e)
			{
				if (*b == ' ' || *b == '\n')
				{
					if ((size_t)(b - c) >= w)
					{
						if (d == statusText.end())
							d=b;
						break;
					}

					if (*b == '\n')
					{
						d=b;
						break;
					}

					d=b;
				}
				b++;
			}

			if (b == e && (size_t)(b - c) < w)
				d=b;

			b=d;

			vector<wchar_t> line;

			line.insert(line.end(), c, b);
			line.push_back(0);

			extendedErrorMsg.push_back(line);
			if (b != e)
				b++;
		}
		clearStatusTimer.cancelTimer();
		statusText.clear();
		parentScreen->resized();
		return;
	}

	clearStatusTimer.cancelTimer();
	draw();
}

void CursesStatusBar::clearstatus()
{
	if (fieldActive != NULL)
		return;

	statusText.clear();
	progressText.clear();
	busyCounter=0;
	if (extendedErrorMsg.size() > 0)
	{
		extendedErrorMsg.clear();
		parentScreen->resized();
		return;
	}
	draw();
}

void CursesStatusBar::busy()
{
	++busyCounter;
	draw();
}

void CursesStatusBar::notbusy()
{
	busyCounter=0;
	draw();
}

bool CursesStatusBar::processKey(const Curses::Key &key)
{
	if (key == shortcut_next_keycode)
	{
		if (++currentShortcutPage >= shortcuts.size())
			currentShortcutPage=0;
		draw();
		return true;

	}

	if (extendedErrorMsg.size() == 0)
	{
		// Status line messages are erased ten seconds after
		// the first key is received.

		if (statusText.size() > 0 &&
		    clearStatusTimer.expired())
			clearStatusTimer.setTimer(10);

		return false;
	}

	clearstatus(); // Clear wrapped popup
	return true;
}

bool CursesStatusBar::listKeys( vector< pair<string, string> > &list)
{
	if (!fieldActive)
		return false;

	list.insert(list.end(), fieldActive->getOptionHelp().begin(),
		    fieldActive->getOptionHelp().end());
	return true;
}

CursesField *CursesStatusBar::createPrompt(string prompt, string initvalue)
{
	if (fieldActive) // Old prompt?
	{
		Curses *f=fieldActive;

		fieldActive=NULL;
		delete f;
	}

	// The following requestFocus() may end up invoking focusLost()
	// method of another Curses object.  That, in turn, _may_ run some
	// code that creates another dialog prompt.  We'd rather that this
	// happen at THIS point.  Otherwise, the second dialog prompt will
	// now blow away this dialog.

	dropFocus();

	clearstatus();
	mbtow(prompt.c_str(), statusText);

	size_t maxW=getWidth();

	if (maxW > 20)
		maxW -= 20;
	else
		maxW=0;

	if (statusText.size() > maxW)
		statusText.erase(statusText.begin() + maxW, statusText.end());

	fieldActive=new Field(this, getWidth() - statusText.size() - 2,
			      255, initvalue);
	if (!fieldActive)
		return NULL;

	fieldActive->setAttribute(attrStatusBar);

	CursesKeyHandler::handlerListModified=true;
	// Rebuild shortcuts

	fieldActive->setCol(statusText.size()+2);
	draw();
	if (fieldActive)
		fieldActive->requestFocus();
	return fieldActive;
}

// Callback from Field, when ENTER is pressed

void CursesStatusBar::fieldEnter()
{
	fieldValue=fieldActive->getText();
	delete fieldActive;
	fieldActive=NULL;
	fieldAborted=false;
	CursesKeyHandler::handlerListModified=true;
	keepgoing=false;
	clearstatus();
}

void CursesStatusBar::fieldAbort()
{
	fieldValue="";
	delete fieldActive;
	fieldActive=NULL;
	fieldAborted=true;
	CursesKeyHandler::handlerListModified=true;
	keepgoing=false;
	clearstatus();
}

//////////////////////////////////////////////////////////////////////
//
// The custom field

CursesStatusBar::Field::Field(CursesStatusBar *parent, size_t widthArg,
			      size_t maxlengthArg,
			      string initValue)
	: CursesField(parent, widthArg, maxlengthArg, initValue),
	  me(parent)
{
}

CursesStatusBar::Field::~Field()
{
}

bool CursesStatusBar::Field::processKeyInFocus(const Key &key)
{
	if (key == '\t' || key == key.DOWN || key == key.UP ||
	    key == key.SHIFTDOWN || key == key.SHIFTUP ||
	    key == key.PGDN || key == key.PGUP ||
	    key == key.SHIFTPGDN || key == key.SHIFTPGUP)

		return true;	// No changing focus, please.

	if (key == '\x03')
	{
		setText("");
		me->fieldAbort();
		return true;
	}

	if (key == key.ENTER)
	{
		me->fieldEnter();
		return true;
	}

	return CursesField::processKeyInFocus(key);
}

bool CursesStatusBar::Field::writeText(const char *text, int row, int col,
				       const CursesAttr &attr) const
{
	CursesAttr attr_cpy=attr;
	attr_cpy.setReverse(!attr_cpy.getReverse());
	attr_cpy.setUnderline(false);

	return CursesField::writeText(text, row, col, attr_cpy);
}

bool CursesStatusBar::Field::writeText(const wchar_t *text, int row, int col,
				       const CursesAttr &attr) const
{
	CursesAttr attr_cpy=attr;
	attr_cpy.setReverse(!attr_cpy.getReverse());
	attr_cpy.setUnderline(false);

	return CursesField::writeText(text, row, col, attr_cpy);
}

