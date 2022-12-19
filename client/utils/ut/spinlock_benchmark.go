package ut

import (
	"sync"
	"testing"

	"github.com/sazikov-ad/networks/client/utils"
)

func BenchmarkMutexUncontended(b *testing.B) {
	type PaddedMutex struct {
		sync.Mutex
		pad [128]uint8
	}
	b.RunParallel(func(pb *testing.PB) {
		var mu PaddedMutex
		for pb.Next() {
			mu.Lock()
			mu.Unlock()
		}
	})
}

func BenchmarkSpinLockUncontended(b *testing.B) {
	type PaddedSpinLock struct {
		utils.SpinLock
		pad [128]uint8
	}
	b.RunParallel(func(pb *testing.PB) {
		var mu PaddedSpinLock
		for pb.Next() {
			mu.Lock()
			mu.Unlock()
		}
	})
}

func benchmarkMutex(b *testing.B, slack, work bool) {
	var mu sync.Mutex
	if slack {
		b.SetParallelism(10)
	}
	b.RunParallel(func(pb *testing.PB) {
		foo := 0
		for pb.Next() {
			mu.Lock()
			mu.Unlock()
			if work {
				for i := 0; i < 100; i++ {
					foo *= 2
					foo /= 2
				}
			}
		}
		_ = foo
	})
}

func benchmarkSpinLock(b *testing.B, slack, work bool) {
	var mu utils.SpinLock
	if slack {
		b.SetParallelism(10)
	}
	b.RunParallel(func(pb *testing.PB) {
		foo := 0
		for pb.Next() {
			mu.Lock()
			mu.Unlock()
			if work {
				for i := 0; i < 100; i++ {
					foo *= 2
					foo /= 2
				}
			}
		}
		_ = foo
	})
}

func BenchmarkMutex(b *testing.B) {
	benchmarkMutex(b, false, false)
}

func BenchmarkSpinLock(b *testing.B) {
	benchmarkSpinLock(b, false, false)
}

func BenchmarkMutexSlack(b *testing.B) {
	benchmarkMutex(b, true, false)
}

func BenchmarkSpinLockSlack(b *testing.B) {
	benchmarkSpinLock(b, true, false)
}

func BenchmarkMutexWork(b *testing.B) {
	benchmarkMutex(b, false, true)
}

func BenchmarkSpinLockWork(b *testing.B) {
	benchmarkSpinLock(b, false, true)
}

func BenchmarkMutexWorkSlack(b *testing.B) {
	benchmarkMutex(b, true, true)
}

func BenchmarkSpinLockWorkSlack(b *testing.B) {
	benchmarkSpinLock(b, true, true)
}
