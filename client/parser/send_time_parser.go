package parser

import (
	"errors"
	"strconv"
	"strings"
	"time"
)

func daysInMonth(month, year int) int {
	switch time.Month(month) {
	case time.April, time.June, time.September, time.November:
		return 30
	case time.February:
		if year%4 == 0 && (year%100 != 0 || year%400 == 0) { // leap year
			return 29
		}
		return 28
	default:
		return 31
	}
}

func isDate(date string) bool {
	parts := strings.Split(date, "-")
	if len(parts) != 2 {
		return false
	}
	m, e := strconv.Atoi(parts[0])
	if e != nil || m < 0 || m > 12 {
		return false
	}
	d, e := strconv.Atoi(parts[1])
	if e != nil || d < 0 || d > daysInMonth(m, time.Now().Year()) {
		return false
	}
	return true
}

func isTime(date string) bool {
	parts := strings.Split(date, ":")

	if len(parts) != 2 {
		return false
	}

	h, e := strconv.Atoi(parts[0])

	if e != nil || h < 0 || h > 24 {
		return false
	}

	m, e := strconv.Atoi(parts[1])

	if e != nil || m < 0 || m > 60 {
		return false
	}

	return true
}

func validateSpecificTime(data string) error {
	parts := strings.Split(data, " ")

	hasDate := false
	hasTime := false

	for _, p := range parts {
		if isDate(p) {
			hasDate = true
		}

		if isTime(p) {
			hasTime = true
		}
	}

	if hasDate || hasTime {
		return nil
	} else {
		return errors.New("Invalid format: " + data)
	}
}

func validateAdditionalTime(data string) error {
	data = strings.Trim(data, " ")
	parts := strings.Split(data, ":")
	hasD := false
	hasH := false
	hasM := false

	for _, p := range parts {
		if strings.HasSuffix(p, "d") {
			if v, e := strconv.Atoi(p[:len(p)-1]); e == nil && v >= 0 {
				hasD = true
			} else {
				return errors.New("Invalid format: " + data)
			}
		}
		if strings.HasSuffix(p, "h") {
			if v, e := strconv.Atoi(p[:len(p)-1]); e == nil && v >= 0 {
				hasH = true
			} else {
				return errors.New("Invalid format: " + data)
			}
		}
		if strings.HasSuffix(p, "m") {
			if v, e := strconv.Atoi(p[:len(p)-1]); e == nil && v >= 0 {
				hasM = true
			} else {
				return errors.New("Invalid format: " + data)
			}
		}
	}

	if hasD || hasH || hasM {
		return nil
	} else {
		return errors.New("Invalid format: " + data)
	}
}

func validate(data string) error {
	if len(data) == 0 {
		return errors.New("Invalid data: " + data)
	}
	if data[0] == '+' {
		return validateAdditionalTime(data[1:])
	} else {
		return validateSpecificTime(data)
	}
}

func parseDate(date string) (int, int) {
	parts := strings.Split(date, "-")
	m, _ := strconv.Atoi(parts[0])
	d, _ := strconv.Atoi(parts[1])
	return m, d
}

func parseTime(time string) (int, int) {
	parts := strings.Split(time, ":")
	h, _ := strconv.Atoi(parts[0])
	m, _ := strconv.Atoi(parts[1])
	return h, m
}

func parseAdditionalTime(data string) time.Time {
	data = strings.Trim(data, " ")
	t := time.Now()
	parts := strings.Split(data, " ")

	for _, p := range parts {
		if strings.HasSuffix(p, "d") {
			day, _ := strconv.Atoi(p[:len(p)-1])
			t = t.Add(time.Duration(24*day) * time.Hour)
		}
		if strings.HasSuffix(p, "h") {
			hour, _ := strconv.Atoi(p[:len(p)-1])
			t = t.Add(time.Duration(hour) * time.Hour)
		}
		if strings.HasSuffix(p, "m") {
			minute, _ := strconv.Atoi(p[:len(p)-1])
			t = t.Add(time.Duration(minute) * time.Minute)
		}
	}
	return t
}

func parseSpecificTime(data string) time.Time {
	t := time.Now()
	parts := strings.Split(data, " ")

	for _, p := range parts {
		if isDate(p) {
			m, d := parseDate(p)
			t = time.Date(t.Year(), time.Month(m), d, t.Hour(), t.Minute(), t.Second(), t.Nanosecond(), t.Location())
		} else if isTime(p) {
			h, m := parseTime(p)
			t = time.Date(t.Year(), t.Month(), t.Day(), h, m, t.Second(), t.Nanosecond(), t.Location())
		}
	}

	return t
}

func ParseSendTime(data string) (time.Time, error) {
	data = strings.ToLower(strings.Trim(data, " \t\n"))
	if len(data) == 0 {
		return time.Now(), nil
	}
	if e := validate(data); e != nil {
		return time.Time{}, e
	}
	if data[0] == '+' {
		return parseAdditionalTime(data[1:]), nil
	} else {
		return parseSpecificTime(data), nil
	}
}
