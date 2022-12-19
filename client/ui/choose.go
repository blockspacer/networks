package ui

import "github.com/sazikov-ad/networks/client"

func (u *UserInterface) chooseFirstPage(state *client.State) {
	u.currentPage = FirstPage
	u.pages.SwitchToPage("1")
	u.chooseUserList(state)
}

func (u *UserInterface) chooseSecondPage(state *client.State) {
	u.currentPage = SecondPage
	u.pages.SwitchToPage("2")
	u.application.SetFocus(u.toField)
	u.cleanNewMessageScreen()
}

func (u *UserInterface) chooseThirdPage(state *client.State) {
	u.currentPage = ThirdPage
	u.pages.SwitchToPage("3")
	u.chooseSendList(state)
	u.cleanReplyScreen()
}

func (u *UserInterface) chooseUserList(state *client.State) {
	u.currentSide = LeftSide
	u.application.SetFocus(u.userList)
	u.userMessage.SetText("")
	u.fillUserList(state)
}

func (u *UserInterface) chooseUserMessage(state *client.State) {
	u.currentSide = RightSide
	u.focusOnReply = false
	u.application.SetFocus(u.userMessage)
	u.fillUserMessage(state)
}

func (u *UserInterface) chooseSendList(state *client.State) {
	u.currentSide = LeftSide
	u.application.SetFocus(u.sendList)
	u.sendMessage.SetText("")
	u.fillSendList(state)
}

func (u *UserInterface) chooseSendMessage(state *client.State) {
	u.currentSide = RightSide
	u.application.SetFocus(u.sendMessage)
	u.fillSendMessage()
}

func (u *UserInterface) setCurrentPage(state *client.State) {
	switch u.currentPage {
	case FirstPage:
		u.chooseFirstPage(state)
	case SecondPage:
		u.chooseSecondPage(state)
	case ThirdPage:
		u.chooseThirdPage(state)
	}
}

func (u *UserInterface) setCurrentSide(state *client.State) {
	switch u.currentSide {
	case LeftSide:
		u.chooseUserList(state)
	case RightSide:
		u.chooseUserMessage(state)
	}
}
