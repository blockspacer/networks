package ui

import (
	"sort"
	"strings"

	"github.com/gdamore/tcell/v2"
	"github.com/rivo/tview"
	"github.com/sazikov-ad/networks/client"
	"github.com/sazikov-ad/networks/client/messages"
	pb "github.com/sazikov-ad/networks/proto"
)

func truncateString(str string, num int) string {
	var pos = 0
	for pos < len(str) && pos < num {
		if str[pos] != '\n' {
			pos++
		} else {
			break
		}
	}
	if pos == len(str) {
		return str
	}
	return str[:pos] + "..."
}

func (u *UserInterface) fillUserList(state *client.State) {
	if !u.shouldLoad {
		return
	}

	root := tview.NewTreeNode("Inbox:").SetSelectable(false).SetReference(nil)
	msgs := state.LoadMessages()
	sort.SliceStable(msgs, func(i, j int) bool {
		return msgs[i].GetSendTs() > msgs[j].GetSendTs()
	})

	for _, message := range msgs {
		node := tview.
			NewTreeNode(message.From + ":    " + truncateString(message.Message, 20)).
			SetSelectable(true).
			SetReference(message)
		if state.GetMessageStatus(message) {
			node.SetColor(tcell.ColorWhite)
		} else {
			node.SetColor(tcell.ColorRed)
		}

		root.AddChild(node)
	}

	u.userList.
		SetRoot(root).
		SetCurrentNode(root).
		SetSelectedFunc(func(node *tview.TreeNode) {
			u.userList.SetCurrentNode(node)
			u.fillUserMessage(state)
		})

	u.shouldLoad = false
}

func (u *UserInterface) fillSendList(state *client.State) {
	root := tview.NewTreeNode("Outbox:").SetSelectable(false).SetReference(nil)
	sent := state.LoadSentMessages()

	sort.SliceStable(sent, func(i, j int) bool {
		return sent[i].GetSendTs() > sent[j].GetSendTs()
	})

	for _, message := range sent {
		node := tview.
			NewTreeNode("<" + strings.Join(message.To, ",") + ">:    " + truncateString(message.Message, 20)).
			SetSelectable(true).
			SetReference(message)
		root.AddChild(node)
	}

	u.sendList.
		SetRoot(root).
		SetCurrentNode(root).
		SetSelectedFunc(func(node *tview.TreeNode) {
			u.sendList.SetCurrentNode(node)
			u.fillUserMessage(state)
		})
}

func createMessage(m *pb.Message) string {
	if len(m.Reply) == 0 {
		return m.Message
	}
	return messages.JoinWithReply(m.Message, m.Reply[0])
}

func (u *UserInterface) fillUserMessage(state *client.State) {
	n := u.userList.GetCurrentNode()
	if n != nil {
		p := n.GetReference().(*pb.Message)
		state.SetRead(p.MessageUid)
		u.userList.GetCurrentNode().SetColor(tcell.ColorWhite)
		u.userMessage.SetText(createMessage(p)).ScrollToBeginning()
	}
}

func (u *UserInterface) fillSendMessage() {
	n := u.sendList.GetCurrentNode()
	if n != nil {
		p := n.GetReference().(*pb.Message)
		u.sendMessage.SetText(createMessage(p)).ScrollToBeginning()
	}
}
