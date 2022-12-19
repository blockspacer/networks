package ui

import (
	"time"

	pb "github.com/sazikov-ad/networks/proto"

	"github.com/gdamore/tcell/v2"
	"github.com/rivo/tview"
	"github.com/sazikov-ad/networks/client"
	"github.com/sazikov-ad/networks/client/messages"
	"github.com/sazikov-ad/networks/client/parser"
)

func (u *UserInterface) selectDefaultState(state *client.State) {
	u.setCurrentPage(state)
	u.setCurrentSide(state)
}

func (u *UserInterface) initKeyHandler(state *client.State) {
	u.buttonField.SetSelectedFunc(func() {
		modal := tview.NewModal().AddButtons([]string{"OK"}).SetDoneFunc(func(buttonIndex int, buttonLabel string) {
			if buttonLabel == "OK" {
				u.application.SetRoot(u.pages, true)
			}
		})

		to := u.toField.GetText()
		sendTime := u.sendField.GetText()

		ul, e1 := parser.ParseUserList(to)
		st, e2 := parser.ParseSendTime(sendTime)

		if e1 != nil || e2 != nil {
			s := ""
			if e1 != nil {
				s += e1.Error()
			}
			if e2 != nil {
				s += " " + e2.Error()
			}

			modal.SetText(s)
			u.application.SetRoot(modal, true).SetFocus(modal)
		} else {
			msg := messages.BuildMessage(ul, st, u.messageField.GetText(), nil)
			go func() { state.QueueSendMessage(msg) }()
			u.buttonField.SetBackgroundColor(tcell.ColorGreen)
			u.chooseSecondPage(state)
		}
	})

	u.replyButton.SetSelectedFunc(func() {
		u.onReplyPage = true
		u.focusOnReplyMessage = true
		u.pages.SwitchToPage("4")
		u.application.SetFocus(u.replyField)
	})

	u.sendReplyButton.SetSelectedFunc(func() {
		toM := u.userList.GetCurrentNode().GetReference().(*pb.Message)
		var reply string
		if len(toM.Reply) != 0 {
			reply = messages.JoinWithReply(toM.Message, toM.Reply[0])
		} else {
			reply = toM.Message
		}
		msg := messages.BuildMessage([]string{toM.From}, time.Now(), u.replyField.GetText(), &reply)
		go func() { state.QueueSendMessage(msg) }()
		modal := tview.NewModal().SetText("Send To: " + toM.From + "\nMessage:\n" + u.replyField.GetText()).AddButtons([]string{"OK"}).SetDoneFunc(func(buttonIndex int, buttonLabel string) {
			if buttonLabel == "OK" {
				u.pages.SwitchToPage("1")
				u.application.SetFocus(u.userMessage)
				u.application.SetRoot(u.pages, true)
				u.onReplyPage = false
			}
		})
		u.application.SetRoot(modal, true).SetFocus(modal)
	})

	u.application.SetInputCapture(func(event *tcell.EventKey) *tcell.EventKey {
		switch event.Key() {
		case tcell.KeyCtrlC:
			u.application.Stop()
			return nil
		case tcell.KeyLeft:
			if u.currentPage == FirstPage && u.currentSide == RightSide {
				u.chooseUserList(state)
			} else if u.currentPage == ThirdPage && u.currentSide == RightSide {
				u.chooseSendList(state)
			} else {
				return event
			}
		case tcell.KeyRight:
			if u.currentPage == FirstPage && u.currentSide == LeftSide {
				u.chooseUserMessage(state)
			} else if u.currentPage == ThirdPage && u.currentSide == LeftSide {
				u.chooseSendMessage(state)
			} else {
				return event
			}
		case tcell.KeyF1:
			if u.currentPage != FirstPage {
				u.chooseFirstPage(state)
			} else {
				return event
			}
		case tcell.KeyF2:
			if u.currentPage != SecondPage {
				u.chooseSecondPage(state)
			} else {
				return event
			}
		case tcell.KeyF3:
			if u.currentPage != ThirdPage {
				u.chooseThirdPage(state)
			} else {
				return event
			}
		case tcell.KeyF4:
			if u.currentPage == FirstPage {
				u.shouldLoad = true
				u.chooseUserList(state)
			} else {
				return event
			}
		case tcell.KeyTab:
			if u.currentPage == SecondPage {
				switch u.currentField {
				case ToField:
					u.currentField = TimeField
					u.application.SetFocus(u.sendField)
				case TimeField:
					u.currentField = MessageField
					u.application.SetFocus(u.messageField)
				case MessageField:
					u.currentField = ButtonField
					u.application.SetFocus(u.buttonField)
				case ButtonField:
					u.currentField = ToField
					u.application.SetFocus(u.toField)
				default:
					return event
				}
			} else if u.currentSide == RightSide {
				if u.onReplyPage {
					if u.focusOnReplyMessage {
						u.focusOnReplyMessage = false
						u.application.SetFocus(u.sendReplyButton)
					} else {
						u.focusOnReplyMessage = true
						u.application.SetFocus(u.replyField)
					}
				} else {
					if !u.focusOnReply {
						u.focusOnReply = true
						u.application.SetFocus(u.replyButton)
					} else {
						u.focusOnReply = false
						u.application.SetFocus(u.userMessage)
					}
				}
			} else {
				return event
			}
		default:
			return event
		}
		return nil
	})
}
