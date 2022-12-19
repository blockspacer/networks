package utils

import user2 "os/user"

func CurrentUser() string {
	user, err := user2.Current()
	if err != nil {
		panic(err)
	}
	return user.Username
}
