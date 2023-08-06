/*
 * SHV - Small HyperVisor for testing nested virtualization in hypervisors
 * Copyright (C) 2023  Eric Li
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <xmhf.h>

void spin_lock(spin_lock_t *lock)
{
	while (1) {
		if (*lock == 0) {
			spin_lock_t val = 1;
			asm volatile("lock xchg %0, %1" : "+r"(val), "+m"(*lock));
			if (val == 0) {
				break;
			}
		}
	}
}

void spin_unlock(spin_lock_t *lock)
{
	spin_lock_t val = 0;
	asm volatile("lock xchg %0, %1" : "+r"(val), "+m"(*lock));
}

