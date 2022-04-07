define walk_pt
  # address to be walked
  set $a = $arg0
  set $c0 = vcpu->vmcs.guest_CR0
  set $c3 = vcpu->vmcs.guest_CR3
  set $c4 = vcpu->vmcs.guest_CR4
  set $lmk = ((u64*)vcpu->vmx_vaddr_msr_area_guest)[0]
  set $lmv = ((u64*)vcpu->vmx_vaddr_msr_area_guest)[1]
  if !($c0 & 0x80000000) || $lmk != 0xc0000080
    print "Pre condition failed"
  else
    if !($c4 & 0x20)
      print "32-bit paging"
      print "Not implemented yet"
    else
      if !($lmv & 0x100)
        print "PAE paging"
        print "Not implemented yet"
      else
        if !($c4 & 0x1000)
          print "4-level paging"
          set $p0 =     $c3 & ~0xfff | ((($a >> 39) & 0x1ff) << 3)
          set $e0 = (*(u64*)$p0)
          set $p1 =     $e0 & ~0xfff | ((($a >> 30) & 0x1ff) << 3)
          set $e1 = (*(u64*)$p1)
          if $e1 & (1 << 7)
            print "Page size = 1G"
            print /x $ans = $e1 & ~0x3fffffff | $a & 0x3fffffff
          else
            set $p2 =   $e1 & ~0xfff | ((($a >> 21) & 0x1ff) << 3)
            set $e2 = (*(u64*)$p2)
            if $e2 & (1 << 7)
              print "Page size = 2M"
              print /x $ans = $e2 & ~0x1fffff | $a & 0x1fffff
            else
              set $p3 = $e2 & ~0xfff | ((($a >> 12) & 0x1ff) << 3)
              set $e3 = (*(u64*)$p3)
              print /x $ans = $e3 & ~0xfff | $a & 0xfff
            end
          end
        else
          print "CR4_LA57 not supported"
        end
      end
    end
  end
end
walk_pt vcpu->vmcs.guest_RIP

