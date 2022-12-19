package parser

import (
	"errors"
	"regexp"
	"strings"
)

func ParseUserList(data string) ([]string, error) {
	parsed := parseUserList(data)
	return parsed, validateUserList(parsed)
}

func parseUserList(data string) []string {
	return filter(splitAny(strings.Trim(data, " \t\n,;"), "; ,\t"), func(s string) bool {
		return len(s) != 0
	})
}

func validateUserList(data []string) error {
	var validUserName = regexp.MustCompile(`^[a-z_]([a-z0-9_-]{0,31}|[a-z0-9_-]{0,30}\$)$`)
	for _, u := range data {
		if u[0] != '@' && !validUserName.MatchString(u) {
			return errors.New("Invalid User Name: " + u)
		} else if u[0] == '@' && !validUserName.MatchString(u[1:]) {
			return errors.New("Invalid Group Name: " + u)
		}
	}
	return nil
}
