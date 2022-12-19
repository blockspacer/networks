package ui

import (
	"github.com/rivo/tview"
)

func createUI() *UserInterface {
	u := new(UserInterface)

	u.application = createApplication()
	u.pages = createPages()
	u.userList = createUserList()
	u.userMessage = createUserMessage()

	u.sendList = tview.NewTreeView().SetGraphics(false)
	u.sendMessage = tview.NewTextView().SetWordWrap(true).SetTextAlign(tview.AlignLeft)

	u.replyButton = createReplyButton()
	u.sendReplyButton = createSendReplyButton()

	u.replyField = tview.NewTextArea()
	u.toField = tview.NewTextArea()
	u.sendField = tview.NewTextArea()
	u.messageField = tview.NewTextArea()
	u.buttonField = tview.NewButton("Send Message")
	u.currentPage = FirstPage
	u.currentSide = LeftSide

	u.pages.
		AddPage("1", createGrid(u.userList, u.userMessage, u.replyButton), true, true).
		AddPage("2", createNewMessage(u.toField, u.sendField, u.messageField, u.buttonField), true, true).
		AddPage("3", createSendGrid(u.sendList, u.sendMessage), true, true).
		AddPage("4", createReplyMessage(u.replyField, u.sendReplyButton), true, true)

	u.application.SetRoot(u.pages, true)
	u.shouldLoad = true
	u.focusOnReply = false
	return u
}

func createApplication() *tview.Application {
	return tview.NewApplication()
}

func createPages() *tview.Pages {
	return tview.NewPages()
}

func createUserList() *tview.TreeView {
	return tview.NewTreeView().SetGraphics(false)
}

func createUserMessage() *tview.TextView {
	return tview.NewTextView().SetWordWrap(true).SetTextAlign(tview.AlignLeft)
}

func createReplyButton() *tview.Button {
	return tview.NewButton("Reply")
}

func createSendReplyButton() *tview.Button {
	return tview.NewButton("Send Reply")
}

func createGrid(list *tview.TreeView, content *tview.TextView, reply *tview.Button) *tview.Grid {
	g := tview.
		NewGrid().
		SetRows(10, 0, 1).
		SetColumns(0).
		SetBorders(true).
		AddItem(list, 0, 0, 1, 1, 0, 0, false).
		AddItem(content, 1, 0, 1, 1, 0, 0, false).
		AddItem(reply, 2, 0, 1, 1, 0, 0, false)
	g.SetTitle("Inbox")
	return g
}

func createSendGrid(list *tview.TreeView, content *tview.TextView) *tview.Grid {
	g := tview.
		NewGrid().
		SetRows(10, 0).
		SetColumns(0).
		SetBorders(true).
		AddItem(list, 0, 0, 1, 1, 0, 0, false).
		AddItem(content, 1, 0, 1, 1, 0, 0, false)
	g.SetTitle("Outbox")
	return g
}

func createNewMessage(to *tview.TextArea, send *tview.TextArea, msg *tview.TextArea, b *tview.Button) *tview.Grid {
	toLabel := tview.NewTextView().SetText("\nTo:\n").SetTextAlign(tview.AlignCenter)
	sendTimeLabel := tview.NewTextView().SetText("\nSend Time:\n").SetTextAlign(tview.AlignCenter)
	messageLabel := tview.NewTextView().SetText("\nMessage:\n").SetTextAlign(tview.AlignCenter)

	return tview.
		NewGrid().
		SetRows(3, 3, 0, 3).
		SetColumns(10, 0).
		SetBorders(true).
		AddItem(toLabel, 0, 0, 0, 0, 0, 0, false).
		AddItem(toLabel, 0, 0, 1, 1, 0, 100, false).
		AddItem(sendTimeLabel, 1, 0, 0, 0, 0, 0, false).
		AddItem(sendTimeLabel, 1, 0, 1, 1, 0, 100, false).
		AddItem(messageLabel, 2, 0, 0, 0, 0, 0, false).
		AddItem(messageLabel, 2, 0, 1, 1, 0, 100, false).
		AddItem(to, 0, 1, 1, 1, 0, 0, false).
		AddItem(send, 1, 1, 1, 1, 0, 0, false).
		AddItem(msg, 2, 1, 1, 1, 0, 0, false).
		AddItem(b, 3, 0, 1, 2, 0, 0, false)
}

func createReplyMessage(f *tview.TextArea, r *tview.Button) *tview.Grid {
	messageLabel := tview.NewTextView().SetText("Message:").SetTextAlign(tview.AlignCenter)
	return tview.NewGrid().
		SetRows(1, 0, 3).
		SetColumns(0).
		SetBorders(true).
		AddItem(messageLabel, 0, 0, 1, 1, 0, 0, false).
		AddItem(f, 1, 0, 1, 1, 0, 0, false).
		AddItem(r, 2, 0, 1, 1, 0, 0, false)
}
