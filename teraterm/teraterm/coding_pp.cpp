/*
 * (C) 2020- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* coding property page */

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "tttypes_charset.h"
#include "coding_pp_res.h"
#include "dlglib.h"
#include "compat_win.h"
#include "setting.h"
#include "helpid.h"
#include "ttlib_charset.h"

#include "coding_pp.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

static const char *KanjiInList[] = {"^[$@","^[$B",NULL};
static const char *KanjiOutList[] = {"^[(B","^[(J",NULL};
static const char *KanjiOutList2[] = {"^[(B","^[(J","^[(H",NULL};
static const char *CellWidthList[] = { "1 Cell", "2 Cell", NULL };

struct CodingPPData {
	TTTSet *pts;
	const wchar_t *UILanguageFileW;
	DLGTEMPLATE *dlg_templ;
};

static void EnableWindows(HWND hWnd, const int *list, int count, BOOL enable)
{
	int i;
	for (i = 0; i < count; i++) {
		HWND w = GetDlgItem(hWnd, list[i]);
		EnableWindow(w, enable);
	}
}

static void ArrenageItems(HWND hWnd)
{
	static const int JJISReceiveItems[] = {
		IDC_TERMKINTEXT,
		IDC_TERMKIN,
		IDC_TERMKOUTTEXT,
		IDC_TERMKOUT,
		IDC_TERMKANA,
	};
	static const int JJISSendItems[] = {
		IDC_TERMKANASEND,
	};
	static const int UnicodeItems[] = {
		IDC_AMBIGUOUS_WIDTH_TITLE,
		IDC_AMBIGUOUS_WIDTH_COMBO,
		IDC_EMOJI_WIDTH_CHECK,
		IDC_EMOJI_WIDTH_COMBO,
	};

	// 受信コード
	LRESULT curPos = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
	LRESULT id = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETITEMDATA, curPos, 0);
	LRESULT lang = id / 100;
	LRESULT coding_receive = id % 100;

	// 送信コード
	curPos = SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETCURSEL, 0, 0);
	id = SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETITEMDATA, curPos, 0);
	assert(lang == (id / 100));
	LRESULT coding_send = id % 100;

	bool is_unicode =
		((lang == IdUtf8) ||
		 ((lang == IdJapanese) && ((coding_receive == IdUTF8) || (coding_send == IdUTF8))) ||
		 ((lang == IdKorean) && ((coding_receive == IdUTF8) || (coding_send == IdUTF8))) ||
		 ((lang == IdChinese) && ((coding_receive == IdUTF8) || (coding_send == IdUTF8))));

	// Unicode character width
	if (is_unicode) {
		EnableWindows(hWnd, UnicodeItems, _countof(UnicodeItems), TRUE);
	}
	else {
		EnableWindows(hWnd, UnicodeItems, _countof(UnicodeItems), FALSE);
	}
	if (lang == IdChinese || lang == IdJapanese || lang == IdKorean) {
		SendDlgItemMessage(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO, CB_SETCURSEL, 1, 0);
		CheckDlgButton(hWnd, IDC_EMOJI_WIDTH_CHECK, BST_CHECKED);
		SendDlgItemMessage(hWnd, IDC_EMOJI_WIDTH_COMBO, CB_SETCURSEL, 1, 0);
	}
	else {
		CheckDlgButton(hWnd, IDC_EMOJI_WIDTH_CHECK, BST_UNCHECKED);
		SendDlgItemMessage(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO, CB_SETCURSEL, 0, 0);
	}

	if (lang == IdJapanese && coding_receive == IdJIS) {
		EnableWindows(hWnd, JJISReceiveItems, _countof(JJISReceiveItems), TRUE);
	}
	else {
		EnableWindows(hWnd, JJISReceiveItems, _countof(JJISReceiveItems), FALSE);
	}

	if (lang == IdJapanese && coding_send == IdJIS) {
		EnableWindows(hWnd, JJISSendItems, _countof(JJISSendItems), TRUE);
	}
	else {
		EnableWindows(hWnd, JJISSendItems, _countof(JJISSendItems), FALSE);
	}
}

static INT_PTR CALLBACK Proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_GEN_TITLE" },
		// { IDC_TERMKANJILABEL, "DLG_TERM_KANJI" },
		{ IDC_TERMKANJILABEL, "DLG_TERMK_KANJI" },
		//{ IDC_TERMKANA, "DLG_TERM_KANA" },
		{ IDC_TERMKANJISENDLABEL, "DLG_TERMK_KANJISEND" },
		//{ IDC_TERMKANASEND, "DLG_TERM_KANASEND" },
		{ IDC_TERMKINTEXT, "DLG_TERM_KIN" },
		{ IDC_TERMKOUTTEXT, "DLG_TERM_KOUT" },
		{ IDC_UNICODE2DEC, "DLG_CODING_UNICODE_TO_DEC" },
		{ IDC_DEC2UNICODE, "DLG_CODING_DEC_TO_UNICODE" },
		{ IDC_DEC2UNICODE_BOXDRAWING, "DLG_CODING_UNICODE_TO_DEC_BOXDRAWING" },
		{ IDC_DEC2UNICODE_PUNCTUATION, "DLG_CODING_UNICODE_TO_DEC_PUNCTUATION" },
		{ IDC_DEC2UNICODE_MIDDLEDOT, "DLG_CODING_UNICODE_TO_DEC_MIDDLEDOT" },
	};

	switch (msg) {
		case WM_INITDIALOG: {
			CodingPPData *DlgData = (CodingPPData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
			const TTTSet *ts = DlgData->pts;
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)DlgData);
			SetDlgTextsW(hWnd, TextInfos, _countof(TextInfos), DlgData->pts->UILanguageFileW);

			int recv_index = 0;
			int send_index = 0;
			for (int i = 0;; i++) {
				const TKanjiList *p = GetKanjiList(i);
				if (p == NULL) {
					break;
				}
				int id = p->lang * 100 + p->coding;
				LRESULT index = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_ADDSTRING, 0, (LPARAM)p->CodeName);
				SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_SETITEMDATA, index, id);
				if (ts->Language == p->lang && ts->KanjiCode == p->coding) {
					recv_index = i;
				}

				index =	SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_ADDSTRING, 0, (LPARAM)p->CodeName);
				SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_SETITEMDATA, index, id);
				if (ts->Language == p->lang && ts->KanjiCodeSend == p->coding) {
					send_index = i;
				}
			}
			ExpandCBWidth(hWnd, IDC_TERMKANJI);
			ExpandCBWidth(hWnd, IDC_TERMKANJISEND);
			SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_SETCURSEL, recv_index, 0);
			SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_SETCURSEL, send_index, 0);

			if (recv_index == send_index) {
				CheckDlgButton(hWnd, IDC_USE_DIFFERENT_CODE, BST_UNCHECKED);
				EnableWindow(GetDlgItem(hWnd, IDC_TERMKANJISEND), FALSE);
			}
			else {
				CheckDlgButton(hWnd, IDC_USE_DIFFERENT_CODE, BST_CHECKED);
			}

			SetRB(hWnd, ts->JIS7Katakana, IDC_TERMKANA, IDC_TERMKANA);
			SetRB(hWnd, ts->JIS7KatakanaSend, IDC_TERMKANASEND, IDC_TERMKANASEND);

			{
				const char **kanji_out_list;
				int n;
				n = ts->KanjiIn;
				n = (n <= 0 || 2 < n) ? IdKanjiInB : n;
				SetDropDownList(hWnd, IDC_TERMKIN, KanjiInList, n);

				kanji_out_list = (ts->TermFlag & TF_ALLOWWRONGSEQUENCE) ? KanjiOutList2 : KanjiOutList;
				n = ts->KanjiOut;
				n = (n <= 0 || 3 < n) ? IdKanjiOutB : n;
				SetDropDownList(hWnd, IDC_TERMKOUT, kanji_out_list, n);
			}

			// DEC Special Graphics
			CheckDlgButton(hWnd, IDC_UNICODE2DEC, ts->Dec2Unicode ? BST_UNCHECKED : BST_CHECKED);
			CheckDlgButton(hWnd, IDC_DEC2UNICODE, ts->Dec2Unicode ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_DEC2UNICODE_BOXDRAWING,
						   (ts->UnicodeDecSpMapping & 0x01) != 0 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_DEC2UNICODE_PUNCTUATION,
						   (ts->UnicodeDecSpMapping & 0x02) != 0 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_DEC2UNICODE_MIDDLEDOT,
						   (ts->UnicodeDecSpMapping & 0x04) != 0 ? BST_CHECKED : BST_UNCHECKED);

			ArrenageItems(hWnd);

			// character width
			SetDropDownList(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO, CellWidthList, ts->UnicodeAmbiguousWidth == 1 ? 1 : 2);
			CheckDlgButton(hWnd, IDC_EMOJI_WIDTH_CHECK, ts->UnicodeEmojiOverride ? BST_CHECKED : BST_UNCHECKED);
			SetDropDownList(hWnd, IDC_EMOJI_WIDTH_COMBO, CellWidthList, ts->UnicodeEmojiWidth == 1 ? 1 : 2);

			return TRUE;
		}
		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lp;
			switch (nmhdr->code) {
				case PSN_APPLY: {
					CodingPPData *DlgData = (CodingPPData *)GetWindowLongPtr(hWnd, DWLP_USER);
					TTTSet *ts = DlgData->pts;

					ts->JIS7KatakanaSend = 0;
					ts->JIS7Katakana = 0;
					ts->KanjiIn = 0;
					ts->KanjiOut = 0;

					// 受信コード
					int curPos = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
					int id = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETITEMDATA, curPos, 0);
					int lang = id / 100;
					int coding = id % 100;
					ts->Language = lang;
					ts->KanjiCode = coding;
					if (coding == IdUTF8 && (lang == IdJapanese || lang == IdKorean || lang == IdChinese || lang == IdUtf8)) {
						;
					}
					else if (lang == IdJapanese && (coding == IdSJIS || coding == IdEUC || coding == IdJIS)) {
						if (coding == IdJIS) {
							ts->JIS7Katakana = (IsDlgButtonChecked(hWnd, IDC_TERMKANA) == BST_CHECKED);
							ts->JIS7KatakanaSend = (IsDlgButtonChecked(hWnd, IDC_TERMKANASEND) == BST_CHECKED);
							WORD w = (WORD)GetCurSel(hWnd, IDC_TERMKIN);
							if (w > 0) {
								ts->KanjiIn = w;
							}
							w = (WORD)GetCurSel(hWnd, IDC_TERMKOUT);
							if (w > 0) {
								ts->KanjiOut = w;
							}
						}
					}
					else if (lang == IdRussian &&
							 (coding == IdWindows || coding == IdKOI8 || coding == Id866 || coding == IdISO)) {
					}
					else if (lang == IdKorean && coding == IdKoreanCP949) {
						;
					}
					else if (lang == IdChinese && (coding == IdCnGB2312 || coding == IdCnBig5)) {
						;
					}
					else if (lang == IdEnglish) {
						;
					}
					else {
						assert(FALSE);
					}

					// 送信コード
					curPos = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETCURSEL, 0, 0);
					id = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETITEMDATA, curPos, 0);
					coding = id % 100;
					ts->KanjiCodeSend = coding;

					// characters as wide
					ts->UnicodeAmbiguousWidth = (BYTE)GetCurSel(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO);
					ts->UnicodeEmojiOverride = (BYTE)IsDlgButtonChecked(hWnd, IDC_EMOJI_WIDTH_CHECK);
					ts->UnicodeEmojiWidth = (BYTE)GetCurSel(hWnd, IDC_EMOJI_WIDTH_COMBO);

					// DEC Special Graphics
					ts->Dec2Unicode = (BYTE)IsDlgButtonChecked(hWnd, IDC_DEC2UNICODE);
					ts->UnicodeDecSpMapping =
						(WORD)((IsDlgButtonChecked(hWnd, IDC_DEC2UNICODE_BOXDRAWING) << 0) |
							   (IsDlgButtonChecked(hWnd, IDC_DEC2UNICODE_PUNCTUATION) << 1) |
							   (IsDlgButtonChecked(hWnd, IDC_DEC2UNICODE_MIDDLEDOT) << 2));
					break;
				}
				case PSN_HELP: {
					HWND vtwin = GetParent(hWnd);
					vtwin = GetParent(vtwin);
					PostMessage(vtwin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalCoding, 0);
					break;
				}
				default:
					break;
			}
			break;
		}
		case WM_COMMAND: {
			switch (wp) {
				case IDC_TERMKANJI | (CBN_SELCHANGE << 16): {
					// 受信コード
					LRESULT sel_receive = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
					LRESULT id = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETITEMDATA, sel_receive, 0);
					LRESULT lang_receive = id / 100;
					LRESULT sel_send = SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETCURSEL, 0, 0);
					id = SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETITEMDATA, sel_send, 0);
					LRESULT lang_send = id / 100;

					if (lang_receive != lang_send ||
						IsDlgButtonChecked(hWnd, IDC_USE_DIFFERENT_CODE) == BST_UNCHECKED) {
						// 送信コードを同じ値にする
						SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_SETCURSEL, sel_receive, 0);
					}

					ArrenageItems(hWnd);
					break;
				}
				case IDC_TERMKANJISEND | (CBN_SELCHANGE << 16): {
					// 送信コード
					LRESULT sel_receive = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
					LRESULT id = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETITEMDATA, sel_receive, 0);
					LRESULT lang_receive = id / 100;
					LRESULT sel_send = SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETCURSEL, 0, 0);
					id = SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETITEMDATA, sel_send, 0);
					LRESULT lang_send = id / 100;

					if (lang_receive != lang_send) {
						// 受信コードを同じ値にする
						SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_SETCURSEL, sel_send, 0);
					}

					ArrenageItems(hWnd);
					break;
				}
				case IDC_USE_DIFFERENT_CODE | (BN_CLICKED << 16): {
					const BOOL checked = (IsDlgButtonChecked(hWnd, IDC_USE_DIFFERENT_CODE) == BST_CHECKED) ? TRUE : FALSE;
					EnableWindow(GetDlgItem(hWnd, IDC_TERMKANJISEND), checked ? TRUE : FALSE);
					if (!checked) {
						// USE_DIFFRENT_CODEのチェックが入っていないるとき
						LRESULT recv_index = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
						LRESULT send_index = SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETCURSEL, 0, 0);
						if (recv_index != send_index) {
							// 送受信コードが異なっているなら、送信コードを受信と同一にする
							SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_SETCURSEL, recv_index, 0);
						}
					}
					break;
				}
				case IDC_EMOJI_WIDTH_CHECK | (BN_CLICKED << 16): {
					BOOL enable = (IsDlgButtonChecked(hWnd, IDC_EMOJI_WIDTH_CHECK) == BST_CHECKED) ? TRUE : FALSE;
					EnableWindow(GetDlgItem(hWnd, IDC_EMOJI_WIDTH_COMBO), enable);
					break;
				}
				default:
					break;
			}
			break;
		}
		default:
			return FALSE;
	}
	return FALSE;
}

static UINT CALLBACK CallBack(HWND hwnd, UINT uMsg, struct _PROPSHEETPAGEW *ppsp)
{
	(void)hwnd;
	UINT ret_val = 0;
	switch (uMsg) {
	case PSPCB_CREATE:
		ret_val = 1;
		break;
	case PSPCB_RELEASE:
		free((void *)ppsp->pResource);
		ppsp->pResource = NULL;
		free((void *)ppsp->lParam);
		ppsp->lParam = NULL;
		break;
	default:
		break;
	}
	return ret_val;
}

HPROPSHEETPAGE CodingPageCreate(HINSTANCE inst, TTTSet *pts)
{
	// 注 common/tt_res.h と coding_pp_res.h で値を一致させること
	int id = IDD_TABSHEET_CODING;

	CodingPPData *Param = (CodingPPData *)calloc(1, sizeof(CodingPPData));
	Param->UILanguageFileW = pts->UILanguageFileW;
	Param->pts = pts;

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = CallBack;
	psp.pszTitle = L"Coding";		// TODO lng ファイルに入れる
	psp.pszTemplate = MAKEINTRESOURCEW(id);
#if defined(REWRITE_TEMPLATE)
	psp.dwFlags |= PSP_DLGINDIRECT;
	Param->dlg_templ = TTGetDlgTemplate(inst, MAKEINTRESOURCEA(id));
	psp.pResource = Param->dlg_templ;
#endif

	psp.pfnDlgProc = Proc;
	psp.lParam = (LPARAM)Param;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPCPROPSHEETPAGEW)&psp);
	return hpsp;
}
