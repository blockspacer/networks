package messages

import "strings"

func mapper(vs []string, f func(string) string) []string {
	vsm := make([]string, len(vs))
	for i, v := range vs {
		vsm[i] = f(v)
	}
	return vsm
}

func JoinWithReply(message string, reply string) string {
	r := mapper(strings.Split(reply, "\n"), func(s string) string {
		return "> " + s
	})
	return message + "\n\nIn Reply To:\n" + strings.Join(r, "\n")
}
