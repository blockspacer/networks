package ui

import (
	"github.com/rivo/tview"
	"github.com/sazikov-ad/networks/client"
)

type PageId int
type SideId int
type FieldId int

const (
	FirstPage  PageId = iota
	SecondPage PageId = iota
	ThirdPage  PageId = iota
)

const (
	LeftSide  SideId = iota
	RightSide SideId = iota
)

const (
	ToField      FieldId = iota
	TimeField    FieldId = iota
	MessageField FieldId = iota
	ButtonField  FieldId = iota
)

type UserInterface struct {
	application         *tview.Application
	pages               *tview.Pages
	userList            *tview.TreeView
	userMessage         *tview.TextView
	replyButton         *tview.Button
	sendList            *tview.TreeView
	sendMessage         *tview.TextView
	sendReplyButton     *tview.Button
	replyField          *tview.TextArea
	toField             *tview.TextArea
	sendField           *tview.TextArea
	messageField        *tview.TextArea
	buttonField         *tview.Button
	currentPage         PageId
	currentSide         SideId
	currentField        FieldId
	shouldLoad          bool
	focusOnReply        bool
	onReplyPage         bool
	focusOnReplyMessage bool
}

func CreateUserInterface(state *client.State) *UserInterface {
	u := createUI()
	u.initKeyHandler(state)
	u.selectDefaultState(state)
	return u
}

func (u *UserInterface) Run() {
	if err := u.application.Run(); err != nil {
		panic(err)
	}
}

func (u *UserInterface) Stop() {
	u.application.Stop()
}
