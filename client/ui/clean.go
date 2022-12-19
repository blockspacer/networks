package ui

func (u *UserInterface) cleanReplyScreen() {
	u.replyField.SetText("", true)
}

func (u *UserInterface) cleanNewMessageScreen() {
	u.toField.SetText("", true)
	u.messageField.SetText("", true)
	u.sendField.SetText("", true)
}
