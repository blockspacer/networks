package parser

import "strings"

func splitAny(s, seps string) []string {
	return strings.FieldsFunc(s, func(r rune) bool {
		return strings.ContainsRune(seps, r)
	})
}

func filter(data []string, f func(string) bool) (ret []string) {
	for _, s := range data {
		if f(s) {
			ret = append(ret, s)
		}
	}
	return ret
}
