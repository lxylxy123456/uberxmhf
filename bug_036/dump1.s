
dump1.img：     文件格式 binary


Disassembly of section .data:

00000000 <.data>:
       0:	eb 52                	jmp    0x54
       2:	90                   	nop
       3:	4e                   	dec    %si
       4:	54                   	push   %sp
       5:	46                   	inc    %si
       6:	53                   	push   %bx
       7:	20 20                	and    %ah,(%bx,%si)
       9:	20 20                	and    %ah,(%bx,%si)
       b:	00 02                	add    %al,(%bp,%si)
       d:	08 80 20 00          	or     %al,0x20(%bx,%si)
      11:	b8 5a 00             	mov    $0x5a,%ax
      14:	00 f8                	add    %bh,%al
      16:	00 00                	add    %al,(%bx,%si)
      18:	3f                   	aas    
      19:	00 ff                	add    %bh,%bh
      1b:	00 00                	add    %al,(%bx,%si)
      1d:	08 00                	or     %al,(%bx,%si)
      1f:	00 00                	add    %al,(%bx,%si)
      21:	00 00                	add    %al,(%bx,%si)
      23:	00 80 00 80          	add    %al,-0x8000(%bx,%si)
      27:	00 ff                	add    %bh,%bh
      29:	8f 01                	pop    (%bx,%di)
      2b:	00 00                	add    %al,(%bx,%si)
      2d:	00 00                	add    %al,(%bx,%si)
      2f:	00 aa 10 00          	add    %ch,0x10(%bp,%si)
      33:	00 00                	add    %al,(%bx,%si)
      35:	00 00                	add    %al,(%bx,%si)
      37:	00 02                	add    %al,(%bp,%si)
      39:	00 00                	add    %al,(%bx,%si)
      3b:	00 00                	add    %al,(%bx,%si)
      3d:	00 00                	add    %al,(%bx,%si)
      3f:	00 f6                	add    %dh,%dh
      41:	00 00                	add    %al,(%bx,%si)
      43:	00 01                	add    %al,(%bx,%di)
      45:	00 00                	add    %al,(%bx,%si)
      47:	00 e4                	add    %ah,%ah
      49:	48                   	dec    %ax
      4a:	9c                   	pushf  
      4b:	5c                   	pop    %sp
      4c:	76 9c                	jbe    0xffffffea
      4e:	5c                   	pop    %sp
      4f:	e0 00                	loopne 0x51
      51:	00 00                	add    %al,(%bx,%si)
      53:	00 fa                	add    %bh,%dl
      55:	33 c0                	xor    %ax,%ax
      57:	8e d0                	mov    %ax,%ss
      59:	bc 00 7c             	mov    $0x7c00,%sp
      5c:	fb                   	sti    
      5d:	68 c0 07             	push   $0x7c0
      60:	1f                   	pop    %ds
      61:	1e                   	push   %ds
      62:	68 66 00             	push   $0x66
      65:	cb                   	lret   
      66:	88 16 0e 00          	mov    %dl,0xe
      6a:	66 81 3e 03 00 4e 54 	cmpl   $0x5346544e,0x3
      71:	46 53 
      73:	75 15                	jne    0x8a
      75:	b4 41                	mov    $0x41,%ah
      77:	bb aa 55             	mov    $0x55aa,%bx
      7a:	cd 13                	int    $0x13
      7c:	72 0c                	jb     0x8a
      7e:	81 fb 55 aa          	cmp    $0xaa55,%bx
      82:	75 06                	jne    0x8a
      84:	f7 c1 01 00          	test   $0x1,%cx
      88:	75 03                	jne    0x8d
      8a:	e9 dd 00             	jmp    0x16a
      8d:	1e                   	push   %ds
      8e:	83 ec 18             	sub    $0x18,%sp
      91:	68 1a 00             	push   $0x1a
      94:	b4 48                	mov    $0x48,%ah
      96:	8a 16 0e 00          	mov    0xe,%dl
      9a:	8b f4                	mov    %sp,%si
      9c:	16                   	push   %ss
      9d:	1f                   	pop    %ds
      9e:	cd 13                	int    $0x13
      a0:	9f                   	lahf   
      a1:	83 c4 18             	add    $0x18,%sp
      a4:	9e                   	sahf   
      a5:	58                   	pop    %ax
      a6:	1f                   	pop    %ds
      a7:	72 e1                	jb     0x8a
      a9:	3b 06 0b 00          	cmp    0xb,%ax
      ad:	75 db                	jne    0x8a
      af:	a3 0f 00             	mov    %ax,0xf
      b2:	c1 2e 0f 00 04       	shrw   $0x4,0xf
      b7:	1e                   	push   %ds
      b8:	5a                   	pop    %dx
      b9:	33 db                	xor    %bx,%bx
      bb:	b9 00 20             	mov    $0x2000,%cx
      be:	2b c8                	sub    %ax,%cx
      c0:	66 ff 06 11 00       	incl   0x11
      c5:	03 16 0f 00          	add    0xf,%dx
      c9:	8e c2                	mov    %dx,%es
      cb:	ff 06 16 00          	incw   0x16
      cf:	e8 4b 00             	call   0x11d
      d2:	2b c8                	sub    %ax,%cx
      d4:	77 ef                	ja     0xc5
      d6:	b8 00 bb             	mov    $0xbb00,%ax
      d9:	cd 1a                	int    $0x1a
      db:	66 23 c0             	and    %eax,%eax
      de:	75 2d                	jne    0x10d
      e0:	66 81 fb 54 43 50 41 	cmp    $0x41504354,%ebx
      e7:	75 24                	jne    0x10d
      e9:	81 f9 02 01          	cmp    $0x102,%cx
      ed:	72 1e                	jb     0x10d
      ef:	16                   	push   %ss
      f0:	68 07 bb             	push   $0xbb07
      f3:	16                   	push   %ss
      f4:	68 52 11             	push   $0x1152
      f7:	16                   	push   %ss
      f8:	68 09 00             	push   $0x9
      fb:	66 53                	push   %ebx
      fd:	66 53                	push   %ebx
      ff:	66 55                	push   %ebp
     101:	16                   	push   %ss
     102:	16                   	push   %ss
     103:	16                   	push   %ss
     104:	68 b8 01             	push   $0x1b8
     107:	66 61                	popal  
     109:	0e                   	push   %cs
     10a:	07                   	pop    %es
     10b:	cd 1a                	int    $0x1a
     10d:	33 c0                	xor    %ax,%ax
     10f:	bf 0a 13             	mov    $0x130a,%di
     112:	b9 f6 0c             	mov    $0xcf6,%cx
     115:	fc                   	cld    
     116:	f3 aa                	rep stos %al,%es:(%di)
     118:	e9 fe 01             	jmp    0x319
     11b:	90                   	nop
     11c:	90                   	nop
     11d:	66 60                	pushal 
     11f:	1e                   	push   %ds
     120:	06                   	push   %es
     121:	66 a1 11 00          	mov    0x11,%eax
     125:	66 03 06 1c 00       	add    0x1c,%eax
     12a:	1e                   	push   %ds
     12b:	66 68 00 00 00 00    	pushl  $0x0
     131:	66 50                	push   %eax
     133:	06                   	push   %es
     134:	53                   	push   %bx
     135:	68 01 00             	push   $0x1
     138:	68 10 00             	push   $0x10
     13b:	b4 42                	mov    $0x42,%ah
     13d:	8a 16 0e 00          	mov    0xe,%dl
     141:	16                   	push   %ss
     142:	1f                   	pop    %ds
     143:	8b f4                	mov    %sp,%si
     145:	cd 13                	int    $0x13
     147:	66 59                	pop    %ecx
     149:	5b                   	pop    %bx
     14a:	5a                   	pop    %dx
     14b:	66 59                	pop    %ecx
     14d:	66 59                	pop    %ecx
     14f:	1f                   	pop    %ds
     150:	0f 82 16 00          	jb     0x16a
     154:	66 ff 06 11 00       	incl   0x11
     159:	03 16 0f 00          	add    0xf,%dx
     15d:	8e c2                	mov    %dx,%es
     15f:	ff 0e 16 00          	decw   0x16
     163:	75 bc                	jne    0x121
     165:	07                   	pop    %es
     166:	1f                   	pop    %ds
     167:	66 61                	popal  
     169:	c3                   	ret    

     16a:	a1 f6 01             	mov    0x1f6,%ax
     16d:	e8 09 00             	call   0x179
     170:	a1 fa 01             	mov    0x1fa,%ax
     173:	e8 03 00             	call   0x179
     176:	f4                   	hlt    
     177:	eb fd                	jmp    0x176
     179:	8b f0                	mov    %ax,%si
     17b:	ac                   	lods   %ds:(%si),%al
     17c:	3c 00                	cmp    $0x0,%al
     17e:	74 09                	je     0x189
     180:	b4 0e                	mov    $0xe,%ah
     182:	bb 07 00             	mov    $0x7,%bx
     185:	cd 10                	int    $0x10
     187:	eb f2                	jmp    0x17b
     189:	c3                   	ret    

     18a:	0d 0a 41             	or     $0x410a,%ax
     18d:	20 64 69             	and    %ah,0x69(%si)
     190:	73 6b                	jae    0x1fd
     192:	20 72 65             	and    %dh,0x65(%bp,%si)
     195:	61                   	popa   
     196:	64 20 65 72          	and    %ah,%fs:0x72(%di)
     19a:	72 6f                	jb     0x20b
     19c:	72 20                	jb     0x1be
     19e:	6f                   	outsw  %ds:(%si),(%dx)
     19f:	63 63 75             	arpl   %sp,0x75(%bp,%di)
     1a2:	72 72                	jb     0x216
     1a4:	65 64 00 0d          	gs add %cl,%fs:(%di)
     1a8:	0a 42 4f             	or     0x4f(%bp,%si),%al
     1ab:	4f                   	dec    %di
     1ac:	54                   	push   %sp
     1ad:	4d                   	dec    %bp
     1ae:	47                   	inc    %di
     1af:	52                   	push   %dx
     1b0:	20 69 73             	and    %ch,0x73(%bx,%di)
     1b3:	20 63 6f             	and    %ah,0x6f(%bp,%di)
     1b6:	6d                   	insw   (%dx),%es:(%di)
     1b7:	70 72                	jo     0x22b
     1b9:	65 73 73             	gs jae 0x22f
     1bc:	65 64 00 0d          	gs add %cl,%fs:(%di)
     1c0:	0a 50 72             	or     0x72(%bx,%si),%dl
     1c3:	65 73 73             	gs jae 0x239
     1c6:	20 43 74             	and    %al,0x74(%bp,%di)
     1c9:	72 6c                	jb     0x237
     1cb:	2b 41 6c             	sub    0x6c(%bx,%di),%ax
     1ce:	74 2b                	je     0x1fb
     1d0:	44                   	inc    %sp
     1d1:	65 6c                	gs insb (%dx),%es:(%di)
     1d3:	20 74 6f             	and    %dh,0x6f(%si)
     1d6:	20 72 65             	and    %dh,0x65(%bp,%si)
     1d9:	73 74                	jae    0x24f
     1db:	61                   	popa   
     1dc:	72 74                	jb     0x252
     1de:	0d 0a 00             	or     $0xa,%ax
	...
     1f5:	00 8a 01 a7          	add    %cl,-0x58ff(%bp,%si)
     1f9:	01 bf 01 00          	add    %di,0x1(%bx)
     1fd:	00 55 aa             	add    %dl,-0x56(%di)
     200:	07                   	pop    %es
     201:	00 42 00             	add    %al,0x0(%bp,%si)
     204:	4f                   	dec    %di
     205:	00 4f 00             	add    %cl,0x0(%bx)
     208:	54                   	push   %sp
     209:	00 4d 00             	add    %cl,0x0(%di)
     20c:	47                   	inc    %di
     20d:	00 52 00             	add    %dl,0x0(%bp,%si)
     210:	04 00                	add    $0x0,%al
     212:	24 00                	and    $0x0,%al
     214:	49                   	dec    %cx
     215:	00 33                	add    %dh,(%bp,%di)
     217:	00 30                	add    %dh,(%bx,%si)
     219:	00 00                	add    %al,(%bx,%si)
     21b:	d4 00                	aam    $0x0
     21d:	00 00                	add    %al,(%bx,%si)
     21f:	24 00                	and    $0x0,%al
     221:	00 01                	add    %al,(%bx,%di)
     223:	00 00                	add    %al,(%bx,%si)
     225:	00 08                	add    %cl,(%bx,%si)
     227:	2c 00                	sub    $0x0,%al
     229:	00 08                	add    %cl,(%bx,%si)
     22b:	30 00                	xor    %al,(%bx,%si)
     22d:	00 08                	add    %cl,(%bx,%si)
     22f:	34 00                	xor    $0x0,%al
     231:	00 e8                	add    %ch,%al
     233:	2d 00 00             	sub    $0x0,%ax
     236:	40                   	inc    %ax
     237:	32 00                	xor    (%bx,%si),%al
     239:	00 90 36 00          	add    %dl,0x36(%bx,%si)
     23d:	00 08                	add    %cl,(%bx,%si)
     23f:	38 00                	cmp    %al,(%bx,%si)
     241:	00 00                	add    %al,(%bx,%si)
     243:	00 00                	add    %al,(%bx,%si)
     245:	00 08                	add    %cl,(%bx,%si)
     247:	3c 00                	cmp    $0x0,%al
     249:	00 08                	add    %cl,(%bx,%si)
     24b:	4c                   	dec    %sp
     24c:	00 00                	add    %al,(%bx,%si)
     24e:	08 2c                	or     %ch,(%si)
     250:	00 00                	add    %al,(%bx,%si)
     252:	00 10                	add    %dl,(%bx,%si)
     254:	00 00                	add    %al,(%bx,%si)
     256:	02 00                	add    (%bx,%si),%al
     258:	00 00                	add    %al,(%bx,%si)
     25a:	05 00 4e             	add    $0x4e00,%ax
     25d:	00 54 00             	add    %dl,0x0(%si)
     260:	4c                   	dec    %sp
     261:	00 44 00             	add    %al,0x0(%si)
     264:	52                   	push   %dx
     265:	00 07                	add    %al,(%bx)
     267:	00 42 00             	add    %al,0x0(%bp,%si)
     26a:	4f                   	dec    %di
     26b:	00 4f 00             	add    %cl,0x0(%bx)
     26e:	54                   	push   %sp
     26f:	00 54 00             	add    %dl,0x0(%si)
     272:	47                   	inc    %di
     273:	00 54 00             	add    %dl,0x0(%si)
     276:	07                   	pop    %es
     277:	00 42 00             	add    %al,0x0(%bp,%si)
     27a:	4f                   	dec    %di
     27b:	00 4f 00             	add    %cl,0x0(%bx)
     27e:	54                   	push   %sp
     27f:	00 4e 00             	add    %cl,0x0(%bp)
     282:	58                   	pop    %ax
     283:	00 54 00             	add    %dl,0x0(%si)
     286:	00 04                	add    %al,(%si)
     288:	00 00                	add    %al,(%bx,%si)
     28a:	00 28                	add    %ch,(%bx,%si)
     28c:	00 00                	add    %al,(%bx,%si)
     28e:	00 10                	add    %dl,(%bx,%si)
     290:	00 00                	add    %al,(%bx,%si)
     292:	01 00                	add    %ax,(%bx,%si)
     294:	00 00                	add    %al,(%bx,%si)
     296:	08 00                	or     %al,(%bx,%si)
     298:	00 00                	add    %al,(%bx,%si)
     29a:	0d 0a 41             	or     $0x410a,%ax
     29d:	6e                   	outsb  %ds:(%si),(%dx)
     29e:	20 6f 70             	and    %ch,0x70(%bx)
     2a1:	65 72 61             	gs jb  0x305
     2a4:	74 69                	je     0x30f
     2a6:	6e                   	outsb  %ds:(%si),(%dx)
     2a7:	67 20 73 79          	and    %dh,0x79(%ebx)
     2ab:	73 74                	jae    0x321
     2ad:	65 6d                	gs insw (%dx),%es:(%di)
     2af:	20 77 61             	and    %dh,0x61(%bx)
     2b2:	73 6e                	jae    0x322
     2b4:	27                   	daa    
     2b5:	74 20                	je     0x2d7
     2b7:	66 6f                	outsl  %ds:(%si),(%dx)
     2b9:	75 6e                	jne    0x329
     2bb:	64 2e 20 54 72       	fs and %dl,%cs:0x72(%si)
     2c0:	79 20                	jns    0x2e2
     2c2:	64 69 73 63 6f 6e    	imul   $0x6e6f,%fs:0x63(%bp,%di),%si
     2c8:	6e                   	outsb  %ds:(%si),(%dx)
     2c9:	65 63 74 69          	arpl   %si,%gs:0x69(%si)
     2cd:	6e                   	outsb  %ds:(%si),(%dx)
     2ce:	67 20 61 6e          	and    %ah,0x6e(%ecx)
     2d2:	79 20                	jns    0x2f4
     2d4:	64 72 69             	fs jb  0x340
     2d7:	76 65                	jbe    0x33e
     2d9:	73 20                	jae    0x2fb
     2db:	74 68                	je     0x345
     2dd:	61                   	popa   
     2de:	74 20                	je     0x300
     2e0:	64 6f                	outsw  %fs:(%si),(%dx)
     2e2:	6e                   	outsb  %ds:(%si),(%dx)
     2e3:	27                   	daa    
     2e4:	74 0d                	je     0x2f3
     2e6:	0a 63 6f             	or     0x6f(%bp,%di),%ah
     2e9:	6e                   	outsb  %ds:(%si),(%dx)
     2ea:	74 61                	je     0x34d
     2ec:	69 6e 20 61 6e       	imul   $0x6e61,0x20(%bp),%bp
     2f1:	20 6f 70             	and    %ch,0x70(%bx)
     2f4:	65 72 61             	gs jb  0x358
     2f7:	74 69                	je     0x362
     2f9:	6e                   	outsb  %ds:(%si),(%dx)
     2fa:	67 20 73 79          	and    %dh,0x79(%ebx)
     2fe:	73 74                	jae    0x374
     300:	65 6d                	gs insw (%dx),%es:(%di)
     302:	2e 00 00             	add    %al,%cs:(%bx,%si)
	...
     315:	00 00                	add    %al,(%bx,%si)
     317:	9a 02 66 0f b7       	lcall  $0xb70f,$0x6602
     31c:	06                   	push   %es
     31d:	0b 00                	or     (%bx,%si),%ax
     31f:	66 0f b6 1e 0d 00    	movzbl 0xd,%ebx
     325:	66 f7 e3             	mul    %ebx
     328:	66 a3 52 02          	mov    %eax,0x252
     32c:	66 8b 0e 40 00       	mov    0x40,%ecx
     331:	80 f9 00             	cmp    $0x0,%cl
     334:	0f 8f 0e 00          	jg     0x346
     338:	f6 d9                	neg    %cl
     33a:	66 b8 01 00 00 00    	mov    $0x1,%eax
     340:	66 d3 e0             	shl    %cl,%eax
     343:	eb 08                	jmp    0x34d
     345:	90                   	nop
     346:	66 a1 52 02          	mov    0x252,%eax
     34a:	66 f7 e1             	mul    %ecx
     34d:	66 a3 86 02          	mov    %eax,0x286
     351:	66 0f b7 1e 0b 00    	movzwl 0xb,%ebx
     357:	66 33 d2             	xor    %edx,%edx
     35a:	66 f7 f3             	div    %ebx
     35d:	66 a3 56 02          	mov    %eax,0x256
     361:	e8 a2 04             	call   0x806
     364:	66 8b 0e 4e 02       	mov    0x24e,%ecx
     369:	66 89 0e 26 02       	mov    %ecx,0x226
     36e:	66 03 0e 86 02       	add    0x286,%ecx
     373:	66 89 0e 2a 02       	mov    %ecx,0x22a
     378:	66 03 0e 86 02       	add    0x286,%ecx
     37d:	66 89 0e 2e 02       	mov    %ecx,0x22e
     382:	66 03 0e 86 02       	add    0x286,%ecx
     387:	66 89 0e 3e 02       	mov    %ecx,0x23e
     38c:	66 03 0e 86 02       	add    0x286,%ecx
     391:	66 89 0e 46 02       	mov    %ecx,0x246
     396:	66 b8 90 00 00 00    	mov    $0x90,%eax
     39c:	66 8b 0e 26 02       	mov    0x226,%ecx
     3a1:	e8 90 09             	call   0xd34
     3a4:	66 0b c0             	or     %eax,%eax
     3a7:	0f 84 bf fd          	je     0x16a
     3ab:	66 a3 32 02          	mov    %eax,0x232
     3af:	66 b8 a0 00 00 00    	mov    $0xa0,%eax
     3b5:	66 8b 0e 2a 02       	mov    0x22a,%ecx
     3ba:	e8 77 09             	call   0xd34
     3bd:	66 a3 36 02          	mov    %eax,0x236
     3c1:	66 b8 b0 00 00 00    	mov    $0xb0,%eax
     3c7:	66 8b 0e 2e 02       	mov    0x22e,%ecx
     3cc:	e8 65 09             	call   0xd34
     3cf:	66 a3 3a 02          	mov    %eax,0x23a
     3d3:	66 a1 32 02          	mov    0x232,%eax
     3d7:	66 0b c0             	or     %eax,%eax
     3da:	0f 84 8c fd          	je     0x16a
     3de:	67 80 78 08 00       	cmpb   $0x0,0x8(%eax)
     3e3:	0f 85 83 fd          	jne    0x16a
     3e7:	67 66 8d 50 10       	lea    0x10(%eax),%edx
     3ec:	67 03 42 04          	add    0x4(%edx),%ax
     3f0:	67 66 0f b6 48 0c    	movzbl 0xc(%eax),%ecx
     3f6:	66 89 0e 92 02       	mov    %ecx,0x292
     3fb:	67 66 8b 48 08       	mov    0x8(%eax),%ecx
     400:	66 89 0e 8e 02       	mov    %ecx,0x28e
     405:	66 a1 8e 02          	mov    0x28e,%eax
     409:	66 0f b7 0e 0b 00    	movzwl 0xb,%ecx
     40f:	66 33 d2             	xor    %edx,%edx
     412:	66 f7 f1             	div    %ecx
     415:	66 a3 96 02          	mov    %eax,0x296
     419:	66 a1 46 02          	mov    0x246,%eax
     41d:	66 03 06 8e 02       	add    0x28e,%eax
     422:	66 a3 4a 02          	mov    %eax,0x24a
     426:	66 83 3e 36 02 00    	cmpl   $0x0,0x236
     42c:	0f 84 1d 00          	je     0x44d
     430:	66 83 3e 3a 02 00    	cmpl   $0x0,0x23a
     436:	0f 84 30 fd          	je     0x16a
     43a:	66 8b 1e 3a 02       	mov    0x23a,%ebx
     43f:	1e                   	push   %ds
     440:	07                   	pop    %es
     441:	66 8b 3e 4a 02       	mov    0x24a,%edi
     446:	66 a1 2e 02          	mov    0x22e,%eax
     44a:	e8 ed 01             	call   0x63a
     44d:	e8 2c 0d             	call   0x117c
     450:	66 0b c0             	or     %eax,%eax
     453:	0f 84 03 00          	je     0x45a
     457:	e8 42 0e             	call   0x129c
     45a:	66 0f b7 0e 00 02    	movzwl 0x200,%ecx
     460:	66 b8 02 02 00 00    	mov    $0x202,%eax
     466:	e8 22 08             	call   0xc8b
     469:	66 0b c0             	or     %eax,%eax
     46c:	0f 85 16 00          	jne    0x486
     470:	66 0f b7 0e 5a 02    	movzwl 0x25a,%ecx
     476:	66 b8 5c 02 00 00    	mov    $0x25c,%eax
     47c:	e8 0c 08             	call   0xc8b
     47f:	66 0b c0             	or     %eax,%eax
     482:	0f 84 78 0e          	je     0x12fe
     486:	67 66 8b 00          	mov    (%eax),%eax
     48a:	1e                   	push   %ds
     48b:	07                   	pop    %es
     48c:	66 8b 3e 3e 02       	mov    0x23e,%edi
     491:	e8 3f 06             	call   0xad3
     494:	66 a1 3e 02          	mov    0x23e,%eax
     498:	66 bb 20 00 00 00    	mov    $0x20,%ebx
     49e:	66 b9 00 00 00 00    	mov    $0x0,%ecx
     4a4:	66 ba 00 00 00 00    	mov    $0x0,%edx
     4aa:	e8 e4 00             	call   0x591
     4ad:	66 85 c0             	test   %eax,%eax
     4b0:	0f 85 23 00          	jne    0x4d7
     4b4:	66 a1 3e 02          	mov    0x23e,%eax
     4b8:	66 bb 80 00 00 00    	mov    $0x80,%ebx
     4be:	66 b9 00 00 00 00    	mov    $0x0,%ecx
     4c4:	66 ba 00 00 00 00    	mov    $0x0,%edx
     4ca:	e8 c4 00             	call   0x591
     4cd:	66 0b c0             	or     %eax,%eax
     4d0:	0f 85 44 00          	jne    0x518
     4d4:	e9 27 0e             	jmp    0x12fe
     4d7:	66 33 d2             	xor    %edx,%edx
     4da:	66 b9 80 00 00 00    	mov    $0x80,%ecx
     4e0:	66 a1 3e 02          	mov    0x23e,%eax
     4e4:	e8 ca 08             	call   0xdb1
     4e7:	66 0b c0             	or     %eax,%eax
     4ea:	0f 84 10 0e          	je     0x12fe
     4ee:	1e                   	push   %ds
     4ef:	07                   	pop    %es
     4f0:	66 8b 3e 3e 02       	mov    0x23e,%edi
     4f5:	e8 db 05             	call   0xad3
     4f8:	66 a1 3e 02          	mov    0x23e,%eax
     4fc:	66 bb 80 00 00 00    	mov    $0x80,%ebx
     502:	66 b9 00 00 00 00    	mov    $0x0,%ecx
     508:	66 ba 00 00 00 00    	mov    $0x0,%edx
     50e:	e8 80 00             	call   0x591
     511:	66 0b c0             	or     %eax,%eax
     514:	0f 84 e6 0d          	je     0x12fe
     518:	67 66 0f b7 58 0c    	movzwl 0xc(%eax),%ebx
     51e:	66 81 e3 ff 00 00 00 	and    $0xff,%ebx
     525:	0f 85 db 0d          	jne    0x1304
     529:	66 8b d8             	mov    %eax,%ebx
     52c:	68 00 20             	push   $0x2000
     52f:	07                   	pop    %es
     530:	66 2b ff             	sub    %edi,%edi
     533:	66 a1 3e 02          	mov    0x23e,%eax
     537:	e8 00 01             	call   0x63a
     53a:	68 00 20             	push   $0x2000
     53d:	07                   	pop    %es
     53e:	66 2b ff             	sub    %edi,%edi
     541:	66 a1 3e 02          	mov    0x23e,%eax
     545:	e8 ac 0a             	call   0xff4
     548:	8a 16 0e 00          	mov    0xe,%dl
     54c:	b8 e8 03             	mov    $0x3e8,%ax
     54f:	8e c0                	mov    %ax,%es
     551:	8d 36 0b 00          	lea    0xb,%si
     555:	2b c0                	sub    %ax,%ax
     557:	68 00 20             	push   $0x2000
     55a:	50                   	push   %ax
     55b:	cb                   	lret   
     55c:	06                   	push   %es
     55d:	1e                   	push   %ds
     55e:	66 60                	pushal 
     560:	66 8b da             	mov    %edx,%ebx
     563:	66 0f b6 0e 0d 00    	movzbl 0xd,%ecx
     569:	66 f7 e1             	mul    %ecx
     56c:	66 a3 11 00          	mov    %eax,0x11
     570:	66 8b c3             	mov    %ebx,%eax
     573:	66 f7 e1             	mul    %ecx
     576:	a3 16 00             	mov    %ax,0x16
     579:	8b df                	mov    %di,%bx
     57b:	83 e3 0f             	and    $0xf,%bx
     57e:	8c c0                	mov    %es,%ax
     580:	66 c1 ef 04          	shr    $0x4,%edi
     584:	03 c7                	add    %di,%ax
     586:	50                   	push   %ax
     587:	07                   	pop    %es
     588:	e8 92 fb             	call   0x11d
     58b:	66 61                	popal  
     58d:	90                   	nop
     58e:	1f                   	pop    %ds
     58f:	07                   	pop    %es
     590:	c3                   	ret    

     591:	67 03 40 14          	add    0x14(%eax),%ax
     595:	67 66 83 38 ff       	cmpl   $0xffffffff,(%eax)
     59a:	0f 84 4c 00          	je     0x5ea
     59e:	67 66 39 18          	cmp    %ebx,(%eax)
     5a2:	0f 85 33 00          	jne    0x5d9
     5a6:	66 0b c9             	or     %ecx,%ecx
     5a9:	0f 85 0a 00          	jne    0x5b7
     5ad:	67 80 78 09 00       	cmpb   $0x0,0x9(%eax)
     5b2:	0f 85 23 00          	jne    0x5d9
     5b6:	c3                   	ret    

     5b7:	67 3a 48 09          	cmp    0x9(%eax),%cl
     5bb:	0f 85 1a 00          	jne    0x5d9
     5bf:	66 8b f0             	mov    %eax,%esi
     5c2:	67 03 70 0a          	add    0xa(%eax),%si
     5c6:	e8 97 06             	call   0xc60
     5c9:	66 51                	push   %ecx
     5cb:	1e                   	push   %ds
     5cc:	07                   	pop    %es
     5cd:	66 8b fa             	mov    %edx,%edi
     5d0:	f3 a7                	repz cmpsw %es:(%di),%ds:(%si)
     5d2:	66 59                	pop    %ecx
     5d4:	0f 85 01 00          	jne    0x5d9
     5d8:	c3                   	ret    

     5d9:	67 66 83 78 04 00    	cmpl   $0x0,0x4(%eax)
     5df:	0f 84 07 00          	je     0x5ea
     5e3:	67 66 03 40 04       	add    0x4(%eax),%eax
     5e8:	eb ab                	jmp    0x595
     5ea:	66 2b c0             	sub    %eax,%eax
     5ed:	c3                   	ret    

     5ee:	66 8b f3             	mov    %ebx,%esi
     5f1:	e8 6c 06             	call   0xc60
     5f4:	67 66 03 00          	add    (%eax),%eax
     5f8:	67 f7 40 0c 02 00    	testw  $0x2,0xc(%eax)
     5fe:	0f 85 34 00          	jne    0x636
     602:	67 66 8d 50 10       	lea    0x10(%eax),%edx
     607:	67 3a 4a 40          	cmp    0x40(%edx),%cl
     60b:	0f 85 18 00          	jne    0x627
     60f:	67 66 8d 72 42       	lea    0x42(%edx),%esi
     614:	e8 49 06             	call   0xc60
     617:	66 51                	push   %ecx
     619:	1e                   	push   %ds
     61a:	07                   	pop    %es
     61b:	66 8b fb             	mov    %ebx,%edi
     61e:	f3 a7                	repz cmpsw %es:(%di),%ds:(%si)
     620:	66 59                	pop    %ecx
     622:	0f 85 01 00          	jne    0x627
     626:	c3                   	ret    

     627:	67 83 78 08 00       	cmpw   $0x0,0x8(%eax)
     62c:	0f 84 06 00          	je     0x636
     630:	67 03 40 08          	add    0x8(%eax),%ax
     634:	eb c2                	jmp    0x5f8
     636:	66 33 c0             	xor    %eax,%eax
     639:	c3                   	ret    

     63a:	67 80 7b 08 00       	cmpb   $0x0,0x8(%ebx)
     63f:	0f 85 1c 00          	jne    0x65f
     643:	06                   	push   %es
     644:	1e                   	push   %ds
     645:	66 60                	pushal 
     647:	67 66 8d 53 10       	lea    0x10(%ebx),%edx
     64c:	67 66 8b 0a          	mov    (%edx),%ecx
     650:	66 8b f3             	mov    %ebx,%esi
     653:	67 03 72 04          	add    0x4(%edx),%si
     657:	f3 a4                	rep movsb %ds:(%si),%es:(%di)
     659:	66 61                	popal  
     65b:	90                   	nop
     65c:	1f                   	pop    %ds
     65d:	07                   	pop    %es
     65e:	c3                   	ret    

     65f:	66 50                	push   %eax
     661:	67 66 8d 53 10       	lea    0x10(%ebx),%edx
     666:	66 85 c0             	test   %eax,%eax
     669:	0f 85 0a 00          	jne    0x677
     66d:	67 66 8b 4a 08       	mov    0x8(%edx),%ecx
     672:	66 41                	inc    %ecx
     674:	eb 11                	jmp    0x687
     676:	90                   	nop
     677:	67 66 8b 42 18       	mov    0x18(%edx),%eax
     67c:	66 33 d2             	xor    %edx,%edx
     67f:	66 f7 36 52 02       	divl   0x252
     684:	66 8b c8             	mov    %eax,%ecx
     687:	66 2b c0             	sub    %eax,%eax
     68a:	66 5e                	pop    %esi
     68c:	e8 01 00             	call   0x690
     68f:	c3                   	ret    

     690:	06                   	push   %es
     691:	1e                   	push   %ds
     692:	66 60                	pushal 
     694:	67 80 7b 08 01       	cmpb   $0x1,0x8(%ebx)
     699:	0f 84 03 00          	je     0x6a0
     69d:	e9 ca fa             	jmp    0x16a
     6a0:	66 83 f9 00          	cmp    $0x0,%ecx
     6a4:	0f 85 06 00          	jne    0x6ae
     6a8:	66 61                	popal  
     6aa:	90                   	nop
     6ab:	1f                   	pop    %ds
     6ac:	07                   	pop    %es
     6ad:	c3                   	ret    

     6ae:	66 53                	push   %ebx
     6b0:	66 50                	push   %eax
     6b2:	66 51                	push   %ecx
     6b4:	66 56                	push   %esi
     6b6:	66 57                	push   %edi
     6b8:	06                   	push   %es
     6b9:	e8 91 04             	call   0xb4d
     6bc:	66 8b d1             	mov    %ecx,%edx
     6bf:	07                   	pop    %es
     6c0:	66 5f                	pop    %edi
     6c2:	66 5e                	pop    %esi
     6c4:	66 59                	pop    %ecx
     6c6:	66 85 c0             	test   %eax,%eax
     6c9:	0f 84 34 00          	je     0x701
     6cd:	66 3b ca             	cmp    %edx,%ecx
     6d0:	0f 8d 03 00          	jge    0x6d7
     6d4:	66 8b d1             	mov    %ecx,%edx
     6d7:	e8 82 fe             	call   0x55c
     6da:	66 2b ca             	sub    %edx,%ecx
     6dd:	66 8b da             	mov    %edx,%ebx
     6e0:	66 8b c2             	mov    %edx,%eax
     6e3:	66 0f b6 16 0d 00    	movzbl 0xd,%edx
     6e9:	66 f7 e2             	mul    %edx
     6ec:	66 0f b7 16 0b 00    	movzwl 0xb,%edx
     6f2:	66 f7 e2             	mul    %edx
     6f5:	66 03 f8             	add    %eax,%edi
     6f8:	66 58                	pop    %eax
     6fa:	66 03 c3             	add    %ebx,%eax
     6fd:	66 5b                	pop    %ebx
     6ff:	eb 9f                	jmp    0x6a0
     701:	66 85 f6             	test   %esi,%esi
     704:	0f 84 62 fa          	je     0x16a
     708:	66 51                	push   %ecx
     70a:	66 57                	push   %edi
     70c:	06                   	push   %es
     70d:	67 66 0f b6 43 09    	movzbl 0x9(%ebx),%eax
     713:	66 85 c0             	test   %eax,%eax
     716:	0f 84 20 00          	je     0x73a
     71a:	66 d1 e0             	shl    %eax
     71d:	66 2b e0             	sub    %eax,%esp
     720:	66 8b fc             	mov    %esp,%edi
     723:	66 54                	push   %esp
     725:	66 56                	push   %esi
     727:	67 66 0f b7 73 0a    	movzwl 0xa(%ebx),%esi
     72d:	66 03 f3             	add    %ebx,%esi
     730:	66 8b c8             	mov    %eax,%ecx
     733:	f3 a4                	rep movsb %ds:(%si),%es:(%di)
     735:	66 5e                	pop    %esi
     737:	eb 03                	jmp    0x73c
     739:	90                   	nop
     73a:	66 50                	push   %eax
     73c:	66 50                	push   %eax
     73e:	67 66 8b 03          	mov    (%ebx),%eax
     742:	66 50                	push   %eax
     744:	67 66 8b 43 18       	mov    0x18(%ebx),%eax
     749:	66 50                	push   %eax
     74b:	67 66 8b 56 20       	mov    0x20(%esi),%edx
     750:	66 85 d2             	test   %edx,%edx
     753:	0f 84 0b 00          	je     0x762
     757:	66 8b fe             	mov    %esi,%edi
     75a:	1e                   	push   %ds
     75b:	07                   	pop    %es
     75c:	66 8b c2             	mov    %edx,%eax
     75f:	e8 71 03             	call   0xad3
     762:	66 8b c6             	mov    %esi,%eax
     765:	66 5a                	pop    %edx
     767:	66 59                	pop    %ecx
     769:	66 42                	inc    %edx
     76b:	66 51                	push   %ecx
     76d:	66 56                	push   %esi
     76f:	e8 3f 06             	call   0xdb1
     772:	66 85 c0             	test   %eax,%eax
     775:	0f 84 f1 f9          	je     0x16a
     779:	66 5e                	pop    %esi
     77b:	66 59                	pop    %ecx
     77d:	66 8b fe             	mov    %esi,%edi
     780:	1e                   	push   %ds
     781:	07                   	pop    %es
     782:	e8 4e 03             	call   0xad3
     785:	66 8b c6             	mov    %esi,%eax
     788:	66 8b d9             	mov    %ecx,%ebx
     78b:	66 59                	pop    %ecx
     78d:	66 5a                	pop    %edx
     78f:	66 51                	push   %ecx
     791:	66 56                	push   %esi
     793:	66 d1 e9             	shr    %ecx
     796:	e8 f8 fd             	call   0x591
     799:	66 85 c0             	test   %eax,%eax
     79c:	0f 84 ca f9          	je     0x16a
     7a0:	66 5e                	pop    %esi
     7a2:	66 59                	pop    %ecx
     7a4:	66 03 e1             	add    %ecx,%esp
     7a7:	07                   	pop    %es
     7a8:	66 5f                	pop    %edi
     7aa:	66 59                	pop    %ecx
     7ac:	66 8b d0             	mov    %eax,%edx
     7af:	66 58                	pop    %eax
     7b1:	66 5b                	pop    %ebx
     7b3:	66 8b da             	mov    %edx,%ebx
     7b6:	e9 f5 fe             	jmp    0x6ae
     7b9:	06                   	push   %es
     7ba:	1e                   	push   %ds
     7bb:	66 60                	pushal 
     7bd:	26 67 66 0f b7 5f 04 	movzwl %es:0x4(%edi),%ebx
     7c4:	26 67 66 0f b7 4f 06 	movzwl %es:0x6(%edi),%ecx
     7cb:	66 0b c9             	or     %ecx,%ecx
     7ce:	0f 84 98 f9          	je     0x16a
     7d2:	66 03 df             	add    %edi,%ebx
     7d5:	66 83 c3 02          	add    $0x2,%ebx
     7d9:	66 81 c7 fe 01 00 00 	add    $0x1fe,%edi
     7e0:	66 49                	dec    %ecx
     7e2:	66 0b c9             	or     %ecx,%ecx
     7e5:	0f 84 17 00          	je     0x800
     7e9:	26 67 8b 03          	mov    %es:(%ebx),%ax
     7ed:	26 67 89 07          	mov    %ax,%es:(%edi)
     7f1:	66 83 c3 02          	add    $0x2,%ebx
     7f5:	66 81 c7 00 02 00 00 	add    $0x200,%edi
     7fc:	66 49                	dec    %ecx
     7fe:	eb e2                	jmp    0x7e2
     800:	66 61                	popal  
     802:	90                   	nop
     803:	1f                   	pop    %ds
     804:	07                   	pop    %es
     805:	c3                   	ret    

     806:	06                   	push   %es
     807:	1e                   	push   %ds
     808:	66 60                	pushal 
     80a:	66 b8 01 00 00 00    	mov    $0x1,%eax
     810:	66 a3 22 02          	mov    %eax,0x222
     814:	66 a1 1e 02          	mov    0x21e,%eax
     818:	66 03 06 86 02       	add    0x286,%eax
     81d:	66 a3 8a 02          	mov    %eax,0x28a
     821:	66 03 06 86 02       	add    0x286,%eax
     826:	66 a3 4e 02          	mov    %eax,0x24e
     82a:	66 a1 30 00          	mov    0x30,%eax
     82e:	66 0f b6 1e 0d 00    	movzbl 0xd,%ebx
     834:	66 f7 e3             	mul    %ebx
     837:	66 8b 1e 4e 02       	mov    0x24e,%ebx
     83c:	66 89 07             	mov    %eax,(%bx)
     83f:	66 a3 11 00          	mov    %eax,0x11
     843:	83 c3 04             	add    $0x4,%bx
     846:	66 a1 56 02          	mov    0x256,%eax
     84a:	66 89 07             	mov    %eax,(%bx)
     84d:	a3 16 00             	mov    %ax,0x16
     850:	83 c3 04             	add    $0x4,%bx
     853:	66 89 1e 4e 02       	mov    %ebx,0x24e
     858:	66 8b 1e 1e 02       	mov    0x21e,%ebx
     85d:	1e                   	push   %ds
     85e:	07                   	pop    %es
     85f:	e8 bb f8             	call   0x11d
     862:	66 8b fb             	mov    %ebx,%edi
     865:	e8 51 ff             	call   0x7b9
     868:	66 a1 1e 02          	mov    0x21e,%eax
     86c:	66 bb 20 00 00 00    	mov    $0x20,%ebx
     872:	66 b9 00 00 00 00    	mov    $0x0,%ecx
     878:	66 ba 00 00 00 00    	mov    $0x0,%edx
     87e:	e8 10 fd             	call   0x591
     881:	66 0b c0             	or     %eax,%eax
     884:	0f 84 19 01          	je     0x9a1
     888:	66 8b d8             	mov    %eax,%ebx
     88b:	1e                   	push   %ds
     88c:	07                   	pop    %es
     88d:	66 8b 3e 1a 02       	mov    0x21a,%edi
     892:	66 33 c0             	xor    %eax,%eax
     895:	e8 a2 fd             	call   0x63a
     898:	66 8b 1e 1a 02       	mov    0x21a,%ebx
     89d:	66 81 3f 80 00 00 00 	cmpl   $0x80,(%bx)
     8a4:	0f 84 eb 00          	je     0x993
     8a8:	03 5f 04             	add    0x4(%bx),%bx
     8ab:	eb f0                	jmp    0x89d
     8ad:	66 53                	push   %ebx
     8af:	66 8b 47 10          	mov    0x10(%bx),%eax
     8b3:	66 f7 26 56 02       	mull   0x256
     8b8:	66 50                	push   %eax
     8ba:	66 33 d2             	xor    %edx,%edx
     8bd:	66 0f b6 1e 0d 00    	movzbl 0xd,%ebx
     8c3:	66 f7 f3             	div    %ebx
     8c6:	66 52                	push   %edx
     8c8:	e8 dc 00             	call   0x9a7
     8cb:	66 0b c0             	or     %eax,%eax
     8ce:	0f 84 98 f8          	je     0x16a
     8d2:	66 8b 0e 56 02       	mov    0x256,%ecx
     8d7:	66 0f b6 1e 0d 00    	movzbl 0xd,%ebx
     8dd:	66 f7 e3             	mul    %ebx
     8e0:	66 5a                	pop    %edx
     8e2:	66 03 c2             	add    %edx,%eax
     8e5:	66 8b 1e 4e 02       	mov    0x24e,%ebx
     8ea:	66 89 07             	mov    %eax,(%bx)
     8ed:	83 c3 04             	add    $0x4,%bx
     8f0:	66 0f b6 06 0d 00    	movzbl 0xd,%eax
     8f6:	66 2b c2             	sub    %edx,%eax
     8f9:	66 3b c1             	cmp    %ecx,%eax
     8fc:	0f 86 03 00          	jbe    0x903
     900:	66 8b c1             	mov    %ecx,%eax
     903:	66 89 07             	mov    %eax,(%bx)
     906:	66 2b c8             	sub    %eax,%ecx
     909:	66 5a                	pop    %edx
     90b:	0f 84 75 00          	je     0x984
     90f:	66 03 c2             	add    %edx,%eax
     912:	66 50                	push   %eax
     914:	66 33 d2             	xor    %edx,%edx
     917:	66 0f b6 1e 0d 00    	movzbl 0xd,%ebx
     91d:	66 f7 f3             	div    %ebx
     920:	66 51                	push   %ecx
     922:	e8 82 00             	call   0x9a7
     925:	66 59                	pop    %ecx
     927:	66 0b c0             	or     %eax,%eax
     92a:	0f 84 3c f8          	je     0x16a
     92e:	66 0f b6 1e 0d 00    	movzbl 0xd,%ebx
     934:	66 f7 e3             	mul    %ebx
     937:	66 8b 1e 4e 02       	mov    0x24e,%ebx
     93c:	66 8b 17             	mov    (%bx),%edx
     93f:	83 c3 04             	add    $0x4,%bx
     942:	66 03 17             	add    (%bx),%edx
     945:	66 3b d0             	cmp    %eax,%edx
     948:	0f 85 15 00          	jne    0x961
     94c:	66 0f b6 06 0d 00    	movzbl 0xd,%eax
     952:	66 3b c1             	cmp    %ecx,%eax
     955:	0f 86 03 00          	jbe    0x95c
     959:	66 8b c1             	mov    %ecx,%eax
     95c:	66 01 07             	add    %eax,(%bx)
     95f:	eb a5                	jmp    0x906
     961:	83 c3 04             	add    $0x4,%bx
     964:	66 89 1e 4e 02       	mov    %ebx,0x24e
     969:	66 89 07             	mov    %eax,(%bx)
     96c:	83 c3 04             	add    $0x4,%bx
     96f:	66 0f b6 06 0d 00    	movzbl 0xd,%eax
     975:	66 3b c1             	cmp    %ecx,%eax
     978:	0f 86 03 00          	jbe    0x97f
     97c:	66 8b c1             	mov    %ecx,%eax
     97f:	66 89 07             	mov    %eax,(%bx)
     982:	eb 82                	jmp    0x906
     984:	83 c3 04             	add    $0x4,%bx
     987:	66 ff 06 22 02       	incl   0x222
     98c:	66 89 1e 4e 02       	mov    %ebx,0x24e
     991:	66 5b                	pop    %ebx
     993:	03 5f 04             	add    0x4(%bx),%bx
     996:	66 81 3f 80 00 00 00 	cmpl   $0x80,(%bx)
     99d:	0f 84 0c ff          	je     0x8ad
     9a1:	66 61                	popal  
     9a3:	90                   	nop
     9a4:	1f                   	pop    %ds
     9a5:	07                   	pop    %es
     9a6:	c3                   	ret    

     9a7:	66 8b d0             	mov    %eax,%edx
     9aa:	66 8b 0e 22 02       	mov    0x222,%ecx
     9af:	66 8b 36 8a 02       	mov    0x28a,%esi
     9b4:	66 03 36 86 02       	add    0x286,%esi
     9b9:	66 52                	push   %edx
     9bb:	66 51                	push   %ecx
     9bd:	66 52                	push   %edx
     9bf:	66 8b 1e 8a 02       	mov    0x28a,%ebx
     9c4:	66 8b 3e 56 02       	mov    0x256,%edi
     9c9:	66 8b 04             	mov    (%si),%eax
     9cc:	66 a3 11 00          	mov    %eax,0x11
     9d0:	83 c6 04             	add    $0x4,%si
     9d3:	66 8b 04             	mov    (%si),%eax
     9d6:	a3 16 00             	mov    %ax,0x16
     9d9:	83 c6 04             	add    $0x4,%si
     9dc:	1e                   	push   %ds
     9dd:	07                   	pop    %es
     9de:	e8 3c f7             	call   0x11d
     9e1:	66 2b f8             	sub    %eax,%edi
     9e4:	0f 84 08 00          	je     0x9f0
     9e8:	f7 26 0b 00          	mulw   0xb
     9ec:	03 d8                	add    %ax,%bx
     9ee:	eb d9                	jmp    0x9c9
     9f0:	66 8b 3e 8a 02       	mov    0x28a,%edi
     9f5:	1e                   	push   %ds
     9f6:	07                   	pop    %es
     9f7:	e8 bf fd             	call   0x7b9
     9fa:	66 a1 8a 02          	mov    0x28a,%eax
     9fe:	66 bb 80 00 00 00    	mov    $0x80,%ebx
     a04:	66 b9 00 00 00 00    	mov    $0x0,%ecx
     a0a:	66 8b d1             	mov    %ecx,%edx
     a0d:	e8 81 fb             	call   0x591
     a10:	66 0b c0             	or     %eax,%eax
     a13:	0f 84 53 f7          	je     0x16a
     a17:	66 8b d8             	mov    %eax,%ebx
     a1a:	66 58                	pop    %eax
     a1c:	66 56                	push   %esi
     a1e:	e8 2c 01             	call   0xb4d
     a21:	66 5e                	pop    %esi
     a23:	66 0b c0             	or     %eax,%eax
     a26:	0f 84 05 00          	je     0xa2f
     a2a:	66 5b                	pop    %ebx
     a2c:	66 5b                	pop    %ebx
     a2e:	c3                   	ret    

     a2f:	66 59                	pop    %ecx
     a31:	66 5a                	pop    %edx
     a33:	e2 84                	loop   0x9b9
     a35:	66 33 c0             	xor    %eax,%eax
     a38:	c3                   	ret    

     a39:	06                   	push   %es
     a3a:	1e                   	push   %ds
     a3b:	66 60                	pushal 
     a3d:	66 50                	push   %eax
     a3f:	66 51                	push   %ecx
     a41:	66 33 d2             	xor    %edx,%edx
     a44:	66 0f b6 1e 0d 00    	movzbl 0xd,%ebx
     a4a:	66 f7 f3             	div    %ebx
     a4d:	66 52                	push   %edx
     a4f:	66 57                	push   %edi
     a51:	e8 53 ff             	call   0x9a7
     a54:	66 5f                	pop    %edi
     a56:	66 0b c0             	or     %eax,%eax
     a59:	0f 84 0d f7          	je     0x16a
     a5d:	66 0f b6 1e 0d 00    	movzbl 0xd,%ebx
     a63:	66 f7 e3             	mul    %ebx
     a66:	66 5a                	pop    %edx
     a68:	66 03 c2             	add    %edx,%eax
     a6b:	66 a3 11 00          	mov    %eax,0x11
     a6f:	66 59                	pop    %ecx
     a71:	66 0f b6 1e 0d 00    	movzbl 0xd,%ebx
     a77:	66 3b cb             	cmp    %ebx,%ecx
     a7a:	0f 8e 13 00          	jle    0xa91
     a7e:	89 1e 16 00          	mov    %bx,0x16
     a82:	66 2b cb             	sub    %ebx,%ecx
     a85:	66 58                	pop    %eax
     a87:	66 03 c3             	add    %ebx,%eax
     a8a:	66 50                	push   %eax
     a8c:	66 51                	push   %ecx
     a8e:	eb 14                	jmp    0xaa4
     a90:	90                   	nop
     a91:	66 58                	pop    %eax
     a93:	66 03 c1             	add    %ecx,%eax
     a96:	66 50                	push   %eax
     a98:	89 0e 16 00          	mov    %cx,0x16
     a9c:	66 b9 00 00 00 00    	mov    $0x0,%ecx
     aa2:	66 51                	push   %ecx
     aa4:	06                   	push   %es
     aa5:	66 57                	push   %edi
     aa7:	8b df                	mov    %di,%bx
     aa9:	83 e3 0f             	and    $0xf,%bx
     aac:	8c c0                	mov    %es,%ax
     aae:	66 c1 ef 04          	shr    $0x4,%edi
     ab2:	03 c7                	add    %di,%ax
     ab4:	50                   	push   %ax
     ab5:	07                   	pop    %es
     ab6:	e8 64 f6             	call   0x11d
     ab9:	66 5f                	pop    %edi
     abb:	07                   	pop    %es
     abc:	66 03 3e 52 02       	add    0x252,%edi
     ac1:	66 59                	pop    %ecx
     ac3:	66 58                	pop    %eax
     ac5:	66 83 f9 00          	cmp    $0x0,%ecx
     ac9:	0f 8f 70 ff          	jg     0xa3d
     acd:	66 61                	popal  
     acf:	90                   	nop
     ad0:	1f                   	pop    %ds
     ad1:	07                   	pop    %es
     ad2:	c3                   	ret    

     ad3:	06                   	push   %es
     ad4:	1e                   	push   %ds
     ad5:	66 60                	pushal 
     ad7:	66 f7 26 56 02       	mull   0x256
     adc:	66 8b 0e 56 02       	mov    0x256,%ecx
     ae1:	e8 55 ff             	call   0xa39
     ae4:	e8 d2 fc             	call   0x7b9
     ae7:	66 61                	popal  
     ae9:	90                   	nop
     aea:	1f                   	pop    %ds
     aeb:	07                   	pop    %es
     aec:	c3                   	ret    

     aed:	06                   	push   %es
     aee:	1e                   	push   %ds
     aef:	66 60                	pushal 
     af1:	66 f7 26 92 02       	mull   0x292
     af6:	66 8b 1e 36 02       	mov    0x236,%ebx
     afb:	66 8b 0e 92 02       	mov    0x292,%ecx
     b00:	66 8b 36 2a 02       	mov    0x22a,%esi
     b05:	1e                   	push   %ds
     b06:	07                   	pop    %es
     b07:	66 8b 3e 46 02       	mov    0x246,%edi
     b0c:	e8 81 fb             	call   0x690
     b0f:	e8 a7 fc             	call   0x7b9
     b12:	66 61                	popal  
     b14:	90                   	nop
     b15:	1f                   	pop    %ds
     b16:	07                   	pop    %es
     b17:	c3                   	ret    

     b18:	66 50                	push   %eax
     b1a:	66 53                	push   %ebx
     b1c:	66 51                	push   %ecx
     b1e:	66 8b 1e 4a 02       	mov    0x24a,%ebx
     b23:	66 8b c8             	mov    %eax,%ecx
     b26:	66 c1 e8 03          	shr    $0x3,%eax
     b2a:	66 83 e1 07          	and    $0x7,%ecx
     b2e:	66 03 d8             	add    %eax,%ebx
     b31:	66 b8 01 00 00 00    	mov    $0x1,%eax
     b37:	66 d3 e0             	shl    %cl,%eax
     b3a:	67 84 03             	test   %al,(%ebx)
     b3d:	0f 84 04 00          	je     0xb45
     b41:	f8                   	clc    
     b42:	eb 02                	jmp    0xb46
     b44:	90                   	nop
     b45:	f9                   	stc    
     b46:	66 59                	pop    %ecx
     b48:	66 5b                	pop    %ebx
     b4a:	66 58                	pop    %eax
     b4c:	c3                   	ret    

     b4d:	67 80 7b 08 01       	cmpb   $0x1,0x8(%ebx)
     b52:	0f 84 04 00          	je     0xb5a
     b56:	66 2b c0             	sub    %eax,%eax
     b59:	c3                   	ret    

     b5a:	67 66 8d 73 10       	lea    0x10(%ebx),%esi
     b5f:	67 66 8b 56 08       	mov    0x8(%esi),%edx
     b64:	66 3b c2             	cmp    %edx,%eax
     b67:	0f 87 0b 00          	ja     0xb76
     b6b:	67 66 8b 16          	mov    (%esi),%edx
     b6f:	66 3b c2             	cmp    %edx,%eax
     b72:	0f 83 04 00          	jae    0xb7a
     b76:	66 2b c0             	sub    %eax,%eax
     b79:	c3                   	ret    

     b7a:	67 03 5e 10          	add    0x10(%esi),%bx
     b7e:	66 2b f6             	sub    %esi,%esi
     b81:	67 80 3b 00          	cmpb   $0x0,(%ebx)
     b85:	0f 84 3e 00          	je     0xbc7
     b89:	e8 81 00             	call   0xc0d
     b8c:	66 03 f1             	add    %ecx,%esi
     b8f:	e8 39 00             	call   0xbcb
     b92:	66 03 ca             	add    %edx,%ecx
     b95:	66 3b c1             	cmp    %ecx,%eax
     b98:	0f 8c 21 00          	jl     0xbbd
     b9c:	66 8b d1             	mov    %ecx,%edx
     b9f:	66 50                	push   %eax
     ba1:	67 66 0f b6 0b       	movzbl (%ebx),%ecx
     ba6:	66 8b c1             	mov    %ecx,%eax
     ba9:	66 83 e0 0f          	and    $0xf,%eax
     bad:	66 c1 e9 04          	shr    $0x4,%ecx
     bb1:	66 03 d9             	add    %ecx,%ebx
     bb4:	66 03 d8             	add    %eax,%ebx
     bb7:	66 43                	inc    %ebx
     bb9:	66 58                	pop    %eax
     bbb:	eb c4                	jmp    0xb81
     bbd:	66 2b c8             	sub    %eax,%ecx
     bc0:	66 2b c2             	sub    %edx,%eax
     bc3:	66 03 c6             	add    %esi,%eax
     bc6:	c3                   	ret    

     bc7:	66 2b c0             	sub    %eax,%eax
     bca:	c3                   	ret    

     bcb:	66 2b c9             	sub    %ecx,%ecx
     bce:	67 8a 0b             	mov    (%ebx),%cl
     bd1:	80 e1 0f             	and    $0xf,%cl
     bd4:	66 83 f9 00          	cmp    $0x0,%ecx
     bd8:	0f 85 04 00          	jne    0xbe0
     bdc:	66 2b c9             	sub    %ecx,%ecx
     bdf:	c3                   	ret    

     be0:	66 53                	push   %ebx
     be2:	66 52                	push   %edx
     be4:	66 03 d9             	add    %ecx,%ebx
     be7:	67 66 0f be 13       	movsbl (%ebx),%edx
     bec:	66 49                	dec    %ecx
     bee:	66 4b                	dec    %ebx
     bf0:	66 83 f9 00          	cmp    $0x0,%ecx
     bf4:	0f 84 0d 00          	je     0xc05
     bf8:	66 c1 e2 08          	shl    $0x8,%edx
     bfc:	67 8a 13             	mov    (%ebx),%dl
     bff:	66 4b                	dec    %ebx
     c01:	66 49                	dec    %ecx
     c03:	eb eb                	jmp    0xbf0
     c05:	66 8b ca             	mov    %edx,%ecx
     c08:	66 5a                	pop    %edx
     c0a:	66 5b                	pop    %ebx
     c0c:	c3                   	ret    

     c0d:	66 53                	push   %ebx
     c0f:	66 52                	push   %edx
     c11:	66 2b d2             	sub    %edx,%edx
     c14:	67 8a 13             	mov    (%ebx),%dl
     c17:	66 83 e2 0f          	and    $0xf,%edx
     c1b:	66 2b c9             	sub    %ecx,%ecx
     c1e:	67 8a 0b             	mov    (%ebx),%cl
     c21:	c0 e9 04             	shr    $0x4,%cl
     c24:	66 83 f9 00          	cmp    $0x0,%ecx
     c28:	0f 85 08 00          	jne    0xc34
     c2c:	66 2b c9             	sub    %ecx,%ecx
     c2f:	66 5a                	pop    %edx
     c31:	66 5b                	pop    %ebx
     c33:	c3                   	ret    

     c34:	66 03 da             	add    %edx,%ebx
     c37:	66 03 d9             	add    %ecx,%ebx
     c3a:	67 66 0f be 13       	movsbl (%ebx),%edx
     c3f:	66 49                	dec    %ecx
     c41:	66 4b                	dec    %ebx
     c43:	66 83 f9 00          	cmp    $0x0,%ecx
     c47:	0f 84 0d 00          	je     0xc58
     c4b:	66 c1 e2 08          	shl    $0x8,%edx
     c4f:	67 8a 13             	mov    (%ebx),%dl
     c52:	66 4b                	dec    %ebx
     c54:	66 49                	dec    %ecx
     c56:	eb eb                	jmp    0xc43
     c58:	66 8b ca             	mov    %edx,%ecx
     c5b:	66 5a                	pop    %edx
     c5d:	66 5b                	pop    %ebx
     c5f:	c3                   	ret    

     c60:	66 0b c9             	or     %ecx,%ecx
     c63:	0f 85 01 00          	jne    0xc68
     c67:	c3                   	ret    

     c68:	66 51                	push   %ecx
     c6a:	66 56                	push   %esi
     c6c:	67 83 3e 61          	cmpw   $0x61,(%esi)
     c70:	0f 8c 0c 00          	jl     0xc80
     c74:	67 83 3e 7a          	cmpw   $0x7a,(%esi)
     c78:	0f 8f 04 00          	jg     0xc80
     c7c:	67 83 2e 20          	subw   $0x20,(%esi)
     c80:	66 83 c6 02          	add    $0x2,%esi
     c84:	e2 e6                	loop   0xc6c
     c86:	66 5e                	pop    %esi
     c88:	66 59                	pop    %ecx
     c8a:	c3                   	ret    

     c8b:	66 50                	push   %eax
     c8d:	66 51                	push   %ecx
     c8f:	66 8b d0             	mov    %eax,%edx
     c92:	66 a1 32 02          	mov    0x232,%eax
     c96:	67 66 8d 58 10       	lea    0x10(%eax),%ebx
     c9b:	67 03 43 04          	add    0x4(%ebx),%ax
     c9f:	67 66 8d 40 10       	lea    0x10(%eax),%eax
     ca4:	66 8b da             	mov    %edx,%ebx
     ca7:	e8 44 f9             	call   0x5ee
     caa:	66 0b c0             	or     %eax,%eax
     cad:	0f 84 05 00          	je     0xcb6
     cb1:	66 59                	pop    %ecx
     cb3:	66 59                	pop    %ecx
     cb5:	c3                   	ret    

     cb6:	66 a1 36 02          	mov    0x236,%eax
     cba:	66 0b c0             	or     %eax,%eax
     cbd:	0f 85 08 00          	jne    0xcc9
     cc1:	66 59                	pop    %ecx
     cc3:	66 59                	pop    %ecx
     cc5:	66 33 c0             	xor    %eax,%eax
     cc8:	c3                   	ret    

     cc9:	66 8b 16 36 02       	mov    0x236,%edx
     cce:	67 66 8d 52 10       	lea    0x10(%edx),%edx
     cd3:	67 66 8b 42 18       	mov    0x18(%edx),%eax
     cd8:	66 33 d2             	xor    %edx,%edx
     cdb:	66 f7 36 8e 02       	divl   0x28e
     ce0:	66 33 f6             	xor    %esi,%esi
     ce3:	66 50                	push   %eax
     ce5:	66 56                	push   %esi
     ce7:	66 58                	pop    %eax
     ce9:	66 5e                	pop    %esi
     ceb:	66 3b c6             	cmp    %esi,%eax
     cee:	0f 84 3a 00          	je     0xd2c
     cf2:	66 56                	push   %esi
     cf4:	66 40                	inc    %eax
     cf6:	66 50                	push   %eax
     cf8:	66 48                	dec    %eax
     cfa:	e8 1b fe             	call   0xb18
     cfd:	72 e8                	jb     0xce7
     cff:	e8 eb fd             	call   0xaed
     d02:	66 5a                	pop    %edx
     d04:	66 5e                	pop    %esi
     d06:	66 59                	pop    %ecx
     d08:	66 5b                	pop    %ebx
     d0a:	66 53                	push   %ebx
     d0c:	66 51                	push   %ecx
     d0e:	66 56                	push   %esi
     d10:	66 52                	push   %edx
     d12:	66 a1 46 02          	mov    0x246,%eax
     d16:	67 66 8d 40 18       	lea    0x18(%eax),%eax
     d1b:	e8 d0 f8             	call   0x5ee
     d1e:	66 0b c0             	or     %eax,%eax
     d21:	74 c4                	je     0xce7
     d23:	66 59                	pop    %ecx
     d25:	66 59                	pop    %ecx
     d27:	66 59                	pop    %ecx
     d29:	66 59                	pop    %ecx
     d2b:	c3                   	ret    

     d2c:	66 59                	pop    %ecx
     d2e:	66 59                	pop    %ecx
     d30:	66 33 c0             	xor    %eax,%eax
     d33:	c3                   	ret    

     d34:	66 51                	push   %ecx
     d36:	66 50                	push   %eax
     d38:	66 b8 05 00 00 00    	mov    $0x5,%eax
     d3e:	1e                   	push   %ds
     d3f:	07                   	pop    %es
     d40:	66 8b f9             	mov    %ecx,%edi
     d43:	e8 8d fd             	call   0xad3
     d46:	66 8b c1             	mov    %ecx,%eax
     d49:	66 bb 20 00 00 00    	mov    $0x20,%ebx
     d4f:	66 b9 00 00 00 00    	mov    $0x0,%ecx
     d55:	66 ba 00 00 00 00    	mov    $0x0,%edx
     d5b:	e8 33 f8             	call   0x591
     d5e:	66 5b                	pop    %ebx
     d60:	66 59                	pop    %ecx
     d62:	66 85 c0             	test   %eax,%eax
     d65:	0f 85 15 00          	jne    0xd7e
     d69:	66 8b c1             	mov    %ecx,%eax
     d6c:	66 0f b7 0e 10 02    	movzwl 0x210,%ecx
     d72:	66 ba 12 02 00 00    	mov    $0x212,%edx
     d78:	e8 16 f8             	call   0x591
     d7b:	eb 33                	jmp    0xdb0
     d7d:	90                   	nop
     d7e:	66 33 d2             	xor    %edx,%edx
     d81:	66 8b c1             	mov    %ecx,%eax
     d84:	66 8b cb             	mov    %ebx,%ecx
     d87:	66 50                	push   %eax
     d89:	66 53                	push   %ebx
     d8b:	e8 23 00             	call   0xdb1
     d8e:	66 5b                	pop    %ebx
     d90:	66 5f                	pop    %edi
     d92:	66 0b c0             	or     %eax,%eax
     d95:	0f 84 17 00          	je     0xdb0
     d99:	1e                   	push   %ds
     d9a:	07                   	pop    %es
     d9b:	e8 35 fd             	call   0xad3
     d9e:	66 8b c7             	mov    %edi,%eax
     da1:	66 0f b7 0e 10 02    	movzwl 0x210,%ecx
     da7:	66 ba 12 02 00 00    	mov    $0x212,%edx
     dad:	e8 e1 f7             	call   0x591
     db0:	c3                   	ret    

     db1:	66 52                	push   %edx
     db3:	66 51                	push   %ecx
     db5:	66 bb 20 00 00 00    	mov    $0x20,%ebx
     dbb:	66 b9 00 00 00 00    	mov    $0x0,%ecx
     dc1:	66 ba 00 00 00 00    	mov    $0x0,%edx
     dc7:	e8 c7 f7             	call   0x591
     dca:	66 0b c0             	or     %eax,%eax
     dcd:	0f 84 63 00          	je     0xe34
     dd1:	66 8b d8             	mov    %eax,%ebx
     dd4:	1e                   	push   %ds
     dd5:	07                   	pop    %es
     dd6:	66 8b 3e 1a 02       	mov    0x21a,%edi
     ddb:	66 33 c0             	xor    %eax,%eax
     dde:	e8 59 f8             	call   0x63a
     de1:	1e                   	push   %ds
     de2:	07                   	pop    %es
     de3:	66 8b 1e 1a 02       	mov    0x21a,%ebx
     de8:	66 59                	pop    %ecx
     dea:	66 5a                	pop    %edx
     dec:	26 66 39 0f          	cmp    %ecx,%es:(%bx)
     df0:	0f 85 0c 00          	jne    0xe00
     df4:	26 66 39 57 08       	cmp    %edx,%es:0x8(%bx)
     df9:	0f 84 31 00          	je     0xe2e
     dfd:	eb 13                	jmp    0xe12
     dff:	90                   	nop
     e00:	26 66 83 3f ff       	cmpl   $0xffffffff,%es:(%bx)
     e05:	0f 84 2f 00          	je     0xe38
     e09:	26 83 7f 04 00       	cmpw   $0x0,%es:0x4(%bx)
     e0e:	0f 84 26 00          	je     0xe38
     e12:	26 66 0f b7 47 04    	movzwl %es:0x4(%bx),%eax
     e18:	03 d8                	add    %ax,%bx
     e1a:	8b c3                	mov    %bx,%ax
     e1c:	25 00 80             	and    $0x8000,%ax
     e1f:	74 cb                	je     0xdec
     e21:	8c c0                	mov    %es,%ax
     e23:	05 00 08             	add    $0x800,%ax
     e26:	8e c0                	mov    %ax,%es
     e28:	81 e3 ff 7f          	and    $0x7fff,%bx
     e2c:	eb be                	jmp    0xdec
     e2e:	26 66 8b 47 10       	mov    %es:0x10(%bx),%eax
     e33:	c3                   	ret    

     e34:	66 59                	pop    %ecx
     e36:	66 5a                	pop    %edx
     e38:	66 33 c0             	xor    %eax,%eax
     e3b:	c3                   	ret    

	// ???
     e3c:	66 50                	push   %eax
     e3e:	66 51                	push   %ecx
     e40:	66 8b c7             	mov    %edi,%eax
     e43:	66 c1 e8 04          	shr    $0x4,%eax
     e47:	06                   	push   %es
     e48:	59                   	pop    %cx
     e49:	03 c8                	add    %ax,%cx
     e4b:	51                   	push   %cx
     e4c:	07                   	pop    %es
     e4d:	66 83 e7 0f          	and    $0xf,%edi
     e51:	66 59                	pop    %ecx
     e53:	66 58                	pop    %eax
     e55:	c3                   	ret    

	// On entry: DS = 0x7c0, ES = 0x2000
	// Move 26 bytes from 0x7c0:0xe69 to 0x7c0:0x2000
	// Before entry:
	// 	(gdb) x/26bx 16 * 0x7c0 + 0xe69
	//	0x8a69:	0x01	0x23	0x45	0x67	0x89	0xab	0xcd	0xef
	//	0x8a71:	0xfe	0xdc	0xba	0x98	0x76	0x54	0x32	0x10
	//	0x8a79:	0xf0	0xe1	0xd2	0xc3	0x00	0x00	0x00	0x00
	//	0x8a81:	0x20	0x20
	//	(gdb) x/26bx 16 * 0x7c0 + 0x2000
	//	0x9c00:	0x39	0xf0	0x76	0x0c	0x8b	0x72	0x0c	0x01
	//	0x9c08:	0xd6	0x83	0xc6	0x10	0x39	0xf0	0x76	0x05
	//	0x9c10:	0x8b	0x52	0x04	0xeb	0xe0	0x8d	0x50	0xf0
	//	0x9c18:	0x89	0x13
	//	(gdb) 
     e56:	60                   	pusha  
     e57:	06                   	push   %es
     e58:	be 69 0e             	mov    $0xe69,%si
     e5b:	bf 00 20             	mov    $0x2000,%di
     e5e:	1e                   	push   %ds
     e5f:	07                   	pop    %es
     e60:	b9 0d 00             	mov    $0xd,%cx
     e63:	90                   	nop
     e64:	f3 a5                	rep movsw %ds:(%si),%es:(%di)
     e66:	07                   	pop    %es
     e67:	61                   	popa   
     e68:	c3                   	ret    

     e69:	01 23                	add    %sp,(%bp,%di)
     e6b:	45                   	inc    %bp
     e6c:	67 89 ab cd ef fe dc 	mov    %bp,-0x23011033(%ebx)
     e73:	ba 98 76             	mov    $0x7698,%dx
     e76:	54                   	push   %sp
     e77:	32 10                	xor    (%bx,%si),%dl
     e79:	f0 e1 d2             	lock loope 0xe4e
     e7c:	c3                   	ret    

     e7d:	00 00                	add    %al,(%bx,%si)
     e7f:	00 00                	add    %al,(%bx,%si)
     e81:	20 20                	and    %ah,(%bx,%si)

     e83:	60                   	pusha  
     e84:	8b 36 18 20          	mov    0x2018,%si
     e88:	26 8a 05             	mov    %es:(%di),%al
     e8b:	88 04                	mov    %al,(%si)
     e8d:	47                   	inc    %di
     e8e:	46                   	inc    %si
     e8f:	66 ff 06 14 20       	incl   0x2014
     e94:	81 fe 60 20          	cmp    $0x2060,%si
     e98:	75 06                	jne    0xea0
     e9a:	e8 5b 00             	call   0xef8
     e9d:	be 20 20             	mov    $0x2020,%si
     ea0:	e2 e6                	loop   0xe88
     ea2:	89 36 18 20          	mov    %si,0x2018
     ea6:	61                   	popa   
     ea7:	c3                   	ret    

     ea8:	66 60                	pushal 
     eaa:	8b 36 18 20          	mov    0x2018,%si
     eae:	b0 80                	mov    $0x80,%al
     eb0:	88 04                	mov    %al,(%si)
     eb2:	46                   	inc    %si
     eb3:	32 c0                	xor    %al,%al
     eb5:	81 fe 60 20          	cmp    $0x2060,%si
     eb9:	75 06                	jne    0xec1
     ebb:	e8 3a 00             	call   0xef8
     ebe:	be 20 20             	mov    $0x2020,%si
     ec1:	81 fe 58 20          	cmp    $0x2058,%si
     ec5:	75 e9                	jne    0xeb0
     ec7:	66 33 c0             	xor    %eax,%eax
     eca:	66 a3 58 20          	mov    %eax,0x2058
     ece:	66 a1 14 20          	mov    0x2014,%eax
     ed2:	66 c1 e0 03          	shl    $0x3,%eax
     ed6:	66 0f c8             	bswap  %eax
     ed9:	66 a3 5c 20          	mov    %eax,0x205c
     edd:	e8 18 00             	call   0xef8
     ee0:	bb 00 20             	mov    $0x2000,%bx
     ee3:	66 8b 07             	mov    (%bx),%eax
     ee6:	66 0f c8             	bswap  %eax
     ee9:	66 89 07             	mov    %eax,(%bx)
     eec:	83 c3 04             	add    $0x4,%bx
     eef:	81 fb 34 20          	cmp    $0x2034,%bx
     ef3:	75 ee                	jne    0xee3
     ef5:	66 61                	popal  
     ef7:	c3                   	ret    

	// Some kind of crypto function
	// Input in 0x2020 - 0x2060
	// Output / state maintained in 0x2000 - 0x2014
     ef8:	66 60                	pushal 
     efa:	bb 20 20             	mov    $0x2020,%bx
     efd:	66 8b 07             	mov    (%bx),%eax
     f00:	66 0f c8             	bswap  %eax
     f03:	66 89 07             	mov    %eax,(%bx)
     f06:	83 c3 04             	add    $0x4,%bx
     f09:	81 fb 60 20          	cmp    $0x2060,%bx
     f0d:	75 ee                	jne    0xefd
     f0f:	bb 00 20             	mov    $0x2000,%bx
     f12:	66 8b 0f             	mov    (%bx),%ecx
     f15:	66 8b 57 04          	mov    0x4(%bx),%edx
     f19:	66 8b 77 08          	mov    0x8(%bx),%esi
     f1d:	66 8b 7f 0c          	mov    0xc(%bx),%edi
     f21:	66 8b 6f 10          	mov    0x10(%bx),%ebp
     f25:	bb 20 20             	mov    $0x2020,%bx
     f28:	c7 06 1a 20 dc 0f    	movw   $0xfdc,0x201a
     f2e:	c6 06 1c 20 14       	movb   $0x14,0x201c
     f33:	90                   	nop
     f34:	53                   	push   %bx
     f35:	8b 1e 1a 20          	mov    0x201a,%bx
     f39:	ff 17                	call   *(%bx)
     f3b:	66 03 47 02          	add    0x2(%bx),%eax
     f3f:	5b                   	pop    %bx
     f40:	66 03 e8             	add    %eax,%ebp
     f43:	66 03 2f             	add    (%bx),%ebp
     f46:	66 8b c1             	mov    %ecx,%eax
     f49:	66 c1 c0 05          	rol    $0x5,%eax
     f4d:	66 03 c5             	add    %ebp,%eax
     f50:	66 8b ef             	mov    %edi,%ebp
     f53:	66 8b fe             	mov    %esi,%edi
     f56:	66 8b f2             	mov    %edx,%esi
     f59:	66 c1 c6 1e          	rol    $0x1e,%esi
     f5d:	66 8b d1             	mov    %ecx,%edx
     f60:	66 8b c8             	mov    %eax,%ecx
     f63:	66 8b 07             	mov    (%bx),%eax
     f66:	66 33 47 08          	xor    0x8(%bx),%eax
     f6a:	66 33 47 20          	xor    0x20(%bx),%eax
     f6e:	66 33 47 34          	xor    0x34(%bx),%eax
     f72:	66 d1 c0             	rol    %eax
     f75:	66 89 47 40          	mov    %eax,0x40(%bx)
     f79:	83 c3 04             	add    $0x4,%bx
     f7c:	fe 0e 1c 20          	decb   0x201c
     f80:	75 b2                	jne    0xf34
     f82:	83 06 1a 20 06       	addw   $0x6,0x201a
     f87:	81 3e 1a 20 f4 0f    	cmpw   $0xff4,0x201a
     f8d:	75 9f                	jne    0xf2e
     f8f:	bb 00 20             	mov    $0x2000,%bx
     f92:	66 01 0f             	add    %ecx,(%bx)
     f95:	66 01 57 04          	add    %edx,0x4(%bx)
     f99:	66 01 77 08          	add    %esi,0x8(%bx)
     f9d:	66 01 7f 0c          	add    %edi,0xc(%bx)
     fa1:	66 01 6f 10          	add    %ebp,0x10(%bx)
     fa5:	66 61                	popal  
     fa7:	c3                   	ret    

     fa8:	66 8b c6             	mov    %esi,%eax
     fab:	66 33 c7             	xor    %edi,%eax
     fae:	66 23 c2             	and    %edx,%eax
     fb1:	66 33 c7             	xor    %edi,%eax
     fb4:	c3                   	ret    

     fb5:	66 8b c2             	mov    %edx,%eax
     fb8:	66 33 c6             	xor    %esi,%eax
     fbb:	66 33 c7             	xor    %edi,%eax
     fbe:	c3                   	ret    

     fbf:	66 53                	push   %ebx
     fc1:	66 8b c2             	mov    %edx,%eax
     fc4:	66 23 c6             	and    %esi,%eax
     fc7:	66 8b da             	mov    %edx,%ebx
     fca:	66 23 df             	and    %edi,%ebx
     fcd:	66 0b c3             	or     %ebx,%eax
     fd0:	66 8b de             	mov    %esi,%ebx
     fd3:	66 23 df             	and    %edi,%ebx
     fd6:	66 0b c3             	or     %ebx,%eax
     fd9:	66 5b                	pop    %ebx
     fdb:	c3                   	ret    

     fdc:	a8 0f                	test   $0xf,%al
     fde:	99                   	cwtd   
     fdf:	79 82                	jns    0xf63
     fe1:	5a                   	pop    %dx
     fe2:	b5 0f                	mov    $0xf,%ch
     fe4:	a1 eb d9             	mov    0xd9eb,%ax
     fe7:	6e                   	outsb  %ds:(%si),(%dx)
     fe8:	bf 0f dc             	mov    $0xdc0f,%di
     feb:	bc 1b 8f             	mov    $0x8f1b,%sp
     fee:	b5 0f                	mov    $0xf,%ch
     ff0:	d6                   	(bad)  
     ff1:	c1 62 ca 06          	shlw   $0x6,-0x36(%bp,%si)
     ff5:	1e                   	push   %ds
     ff6:	66 60                	pushal 
     ff8:	66 33 db             	xor    %ebx,%ebx
     ffb:	b8 00 bb             	mov    $0xbb00,%ax
     ffe:	cd 1a                	int    $0x1a
    1000:	66 23 c0             	and    %eax,%eax
    1003:	0f 85 bb 00          	jne    0x10c2
    1007:	66 81 fb 54 43 50 41 	cmp    $0x41504354,%ebx
    100e:	0f 85 b0 00          	jne    0x10c2
    1012:	81 f9 02 01          	cmp    $0x102,%cx
    1016:	0f 82 a8 00          	jb     0x10c2
    101a:	66 61                	popal  
    101c:	90                   	nop
    101d:	1f                   	pop    %ds
    101e:	07                   	pop    %es
    101f:	06                   	push   %es
    1020:	1e                   	push   %ds
    1021:	66 60                	pushal 
    1023:	67 80 7b 08 00       	cmpb   $0x0,0x8(%ebx)
    1028:	0f 85 0c 00          	jne    0x1038
    102c:	67 66 8d 53 10       	lea    0x10(%ebx),%edx
    1031:	67 66 8b 0a          	mov    (%edx),%ecx
    1035:	eb 25                	jmp    0x105c
    1037:	90                   	nop
    1038:	67 66 8d 53 10       	lea    0x10(%ebx),%edx
    103d:	67 66 8b 4a 28       	mov    0x28(%edx),%ecx
    1042:	66 81 f9 00 00 08 00 	cmp    $0x80000,%ecx
    1049:	0f 83 0c 00          	jae    0x1059
    104d:	67 66 8b 42 2c       	mov    0x2c(%edx),%eax
    1052:	66 23 c0             	and    %eax,%eax
    1055:	0f 84 03 00          	je     0x105c
    1059:	66 33 c9             	xor    %ecx,%ecx
    105c:	0e                   	push   %cs
    105d:	1f                   	pop    %ds
    105e:	e8 f5 fd             	call   0xe56			// Move 26 bytes from 0x7c0:0xe69 to 0x7c0:0x2000
    1061:	66 23 c9             	and    %ecx,%ecx
    1064:	0f 84 32 00          	je     0x109a
    1068:	66 ba 00 80 00 00    	mov    $0x8000,%edx
    106e:	66 3b ca             	cmp    %edx,%ecx		// Start loop
    1071:	0f 86 1f 00          	jbe    0x1094
    1075:	66 2b ca             	sub    %edx,%ecx
    1078:	06                   	push   %es
    1079:	66 51                	push   %ecx
    107b:	66 57                	push   %edi
    107d:	66 52                	push   %edx
    107f:	66 8b ca             	mov    %edx,%ecx
    1082:	e8 b7 fd             	call   0xe3c				// Looks like nop?
    1085:	e8 fb fd             	call   0xe83
    1088:	66 5a                	pop    %edx
    108a:	66 5f                	pop    %edi
    108c:	66 59                	pop    %ecx
    108e:	07                   	pop    %es
    108f:	66 03 fa             	add    %edx,%edi
    1092:	eb da                	jmp    0x106e			// End loop
    1094:	e8 a5 fd             	call   0xe3c
    1097:	e8 e9 fd             	call   0xe83
    109a:	e8 0b fe             	call   0xea8
    109d:	0e                   	push   %cs
    109e:	07                   	pop    %es
    109f:	66 bb 54 43 50 41    	mov    $0x41504354,%ebx
    10a5:	66 bf 00 20 00 00    	mov    $0x2000,%edi
    10ab:	66 b9 14 00 00 00    	mov    $0x14,%ecx
    10b1:	66 b8 07 bb 00 00    	mov    $0xbb07,%eax
    10b7:	66 ba 0a 00 00 00    	mov    $0xa,%edx
    10bd:	66 33 f6             	xor    %esi,%esi
    10c0:	cd 1a                	int    $0x1a
    10c2:	66 61                	popal  
    10c4:	90                   	nop
    10c5:	1f                   	pop    %ds
    10c6:	07                   	pop    %es
    10c7:	c3                   	ret    

    10c8:	06                   	push   %es
    10c9:	1e                   	push   %ds
    10ca:	66 60                	pushal 
    10cc:	66 33 db             	xor    %ebx,%ebx
    10cf:	b8 00 bb             	mov    $0xbb00,%ax
    10d2:	cd 1a                	int    $0x1a
    10d4:	66 23 c0             	and    %eax,%eax
    10d7:	0f 85 74 00          	jne    0x114f
    10db:	66 81 fb 54 43 50 41 	cmp    $0x41504354,%ebx
    10e2:	0f 85 69 00          	jne    0x114f
    10e6:	81 f9 02 01          	cmp    $0x102,%cx
    10ea:	0f 82 61 00          	jb     0x114f
    10ee:	66 b8 07 bb 00 00    	mov    $0xbb07,%eax
    10f4:	66 bb 54 43 50 41    	mov    $0x41504354,%ebx
    10fa:	66 b9 b8 01 00 00    	mov    $0x1b8,%ecx
    1100:	66 ba 04 00 00 00    	mov    $0x4,%edx
    1106:	66 33 f6             	xor    %esi,%esi
    1109:	66 be 4d 42 52 43    	mov    $0x4352424d,%esi
    110f:	68 00 00             	push   $0x0
    1112:	07                   	pop    %es
    1113:	66 bf 00 7c 00 00    	mov    $0x7c00,%edi
    1119:	cd 1a                	int    $0x1a
    111b:	66 b8 07 bb 00 00    	mov    $0xbb07,%eax
    1121:	66 bb 54 43 50 41    	mov    $0x41504354,%ebx
    1127:	66 b9 42 00 00 00    	mov    $0x42,%ecx
    112d:	66 ba 05 00 00 00    	mov    $0x5,%edx
    1133:	66 33 f6             	xor    %esi,%esi
    1136:	66 be 4d 42 52 44    	mov    $0x4452424d,%esi
    113c:	68 00 00             	push   $0x0
    113f:	07                   	pop    %es
    1140:	66 bf 00 7c 00 00    	mov    $0x7c00,%edi
    1146:	66 81 c7 be 01 00 00 	add    $0x1be,%edi
    114d:	cd 1a                	int    $0x1a
    114f:	66 61                	popal  
    1151:	90                   	nop
    1152:	1f                   	pop    %ds
    1153:	07                   	pop    %es
    1154:	c3                   	ret    

    1155:	66 52                	push   %edx
    1157:	66 33 c0             	xor    %eax,%eax
    115a:	67 80 7b 08 00       	cmpb   $0x0,0x8(%ebx)
    115f:	0f 85 0c 00          	jne    0x116f
    1163:	67 66 8d 53 10       	lea    0x10(%ebx),%edx
    1168:	67 66 8b 02          	mov    (%edx),%eax
    116c:	eb 0b                	jmp    0x1179
    116e:	90                   	nop
    116f:	67 66 8d 53 10       	lea    0x10(%ebx),%edx
    1174:	67 66 8b 42 28       	mov    0x28(%edx),%eax
    1179:	66 5a                	pop    %edx
    117b:	c3                   	ret    

    117c:	06                   	push   %es
    117d:	1e                   	push   %ds
    117e:	66 60                	pushal 
    1180:	66 0f b7 0e 66 02    	movzwl 0x266,%ecx
    1186:	66 b8 68 02 00 00    	mov    $0x268,%eax
    118c:	e8 fc fa             	call   0xc8b
    118f:	66 0b c0             	or     %eax,%eax
    1192:	0f 84 fa 00          	je     0x1290
    1196:	66 0f b7 0e 76 02    	movzwl 0x276,%ecx
    119c:	66 b8 78 02 00 00    	mov    $0x278,%eax
    11a2:	e8 e6 fa             	call   0xc8b
    11a5:	66 0b c0             	or     %eax,%eax
    11a8:	0f 84 e4 00          	je     0x1290
    11ac:	67 66 8b 00          	mov    (%eax),%eax
    11b0:	1e                   	push   %ds
    11b1:	07                   	pop    %es
    11b2:	66 8b 3e 3e 02       	mov    0x23e,%edi
    11b7:	e8 19 f9             	call   0xad3
    11ba:	66 a1 3e 02          	mov    0x23e,%eax
    11be:	66 bb 20 00 00 00    	mov    $0x20,%ebx
    11c4:	66 b9 00 00 00 00    	mov    $0x0,%ecx
    11ca:	66 ba 00 00 00 00    	mov    $0x0,%edx
    11d0:	e8 be f3             	call   0x591
    11d3:	66 85 c0             	test   %eax,%eax
    11d6:	0f 85 23 00          	jne    0x11fd
    11da:	66 a1 3e 02          	mov    0x23e,%eax
    11de:	66 bb 80 00 00 00    	mov    $0x80,%ebx
    11e4:	66 b9 00 00 00 00    	mov    $0x0,%ecx
    11ea:	66 ba 00 00 00 00    	mov    $0x0,%edx
    11f0:	e8 9e f3             	call   0x591
    11f3:	66 0b c0             	or     %eax,%eax
    11f6:	0f 85 44 00          	jne    0x123e
    11fa:	e9 93 00             	jmp    0x1290
    11fd:	66 33 d2             	xor    %edx,%edx
    1200:	66 b9 80 00 00 00    	mov    $0x80,%ecx
    1206:	66 a1 3e 02          	mov    0x23e,%eax
    120a:	e8 a4 fb             	call   0xdb1
    120d:	66 0b c0             	or     %eax,%eax
    1210:	0f 84 7c 00          	je     0x1290
    1214:	1e                   	push   %ds
    1215:	07                   	pop    %es
    1216:	66 8b 3e 3e 02       	mov    0x23e,%edi
    121b:	e8 b5 f8             	call   0xad3
    121e:	66 a1 3e 02          	mov    0x23e,%eax
    1222:	66 bb 80 00 00 00    	mov    $0x80,%ebx
    1228:	66 b9 00 00 00 00    	mov    $0x0,%ecx
    122e:	66 ba 00 00 00 00    	mov    $0x0,%edx
    1234:	e8 5a f3             	call   0x591
    1237:	66 0b c0             	or     %eax,%eax
    123a:	0f 84 52 00          	je     0x1290
    123e:	66 8b d0             	mov    %eax,%edx
    1241:	66 8b d8             	mov    %eax,%ebx
    1244:	e8 0e ff             	call   0x1155
    1247:	66 3d 00 04 00 00    	cmp    $0x400,%eax
    124d:	0f 87 3f 00          	ja     0x1290
    1251:	67 66 0f b7 5a 0c    	movzwl 0xc(%edx),%ebx
    1257:	66 81 e3 ff 00 00 00 	and    $0xff,%ebx
    125e:	0f 85 2e 00          	jne    0x1290
    1262:	66 8b da             	mov    %edx,%ebx
    1265:	68 00 20             	push   $0x2000
    1268:	07                   	pop    %es
    1269:	66 2b ff             	sub    %edi,%edi
    126c:	66 a1 3e 02          	mov    0x23e,%eax
    1270:	e8 c7 f3             	call   0x63a
    1273:	68 00 20             	push   $0x2000
    1276:	1f                   	pop    %ds
    1277:	66 33 d2             	xor    %edx,%edx
    127a:	67 8a 0a             	mov    (%edx),%cl
    127d:	80 f9 00             	cmp    $0x0,%cl
    1280:	0f 84 0c 00          	je     0x1290
    1284:	66 61                	popal  
    1286:	90                   	nop
    1287:	1f                   	pop    %ds
    1288:	07                   	pop    %es
    1289:	66 b8 01 00 00 00    	mov    $0x1,%eax
    128f:	c3                   	ret    

    1290:	66 61                	popal  
    1292:	90                   	nop
    1293:	1f                   	pop    %ds
    1294:	07                   	pop    %es
    1295:	66 b8 00 00 00 00    	mov    $0x0,%eax
    129b:	c3                   	ret    

    129c:	06                   	push   %es
    129d:	1e                   	push   %ds
    129e:	66 60                	pushal 
    12a0:	8a 16 0e 00          	mov    0xe,%dl
    12a4:	fe c2                	inc    %dl
    12a6:	2b 26 0b 00          	sub    0xb,%sp
    12aa:	b8 01 02             	mov    $0x201,%ax
    12ad:	32 ed                	xor    %ch,%ch
    12af:	b1 01                	mov    $0x1,%cl
    12b1:	32 f6                	xor    %dh,%dh
    12b3:	16                   	push   %ss
    12b4:	07                   	pop    %es
    12b5:	8b dc                	mov    %sp,%bx
    12b7:	cd 13                	int    $0x13
    12b9:	0f 82 37 00          	jb     0x12f4
    12bd:	b9 04 00             	mov    $0x4,%cx
    12c0:	8b eb                	mov    %bx,%bp
    12c2:	81 c5 be 01          	add    $0x1be,%bp
    12c6:	80 7e 00 00          	cmpb   $0x0,0x0(%bp)
    12ca:	0f 8c 0b 00          	jl     0x12d9
    12ce:	83 c5 10             	add    $0x10,%bp
    12d1:	e2 f3                	loop   0x12c6
    12d3:	03 26 0b 00          	add    0xb,%sp
    12d7:	eb cb                	jmp    0x12a4
    12d9:	68 00 00             	push   $0x0
    12dc:	07                   	pop    %es
    12dd:	16                   	push   %ss
    12de:	1f                   	pop    %ds
    12df:	8b f3                	mov    %bx,%si
    12e1:	bf 00 7c             	mov    $0x7c00,%di
    12e4:	b9 00 02             	mov    $0x200,%cx
    12e7:	fc                   	cld    
    12e8:	f3 a4                	rep movsb %ds:(%si),%es:(%di)
    12ea:	e8 db fd             	call   0x10c8
    12ed:	68 00 00             	push   $0x0
    12f0:	68 00 7c             	push   $0x7c00
    12f3:	cb                   	lret   
    12f4:	03 26 0b 00          	add    0xb,%sp
    12f8:	66 61                	popal  
    12fa:	90                   	nop
    12fb:	1f                   	pop    %ds
    12fc:	07                   	pop    %es
    12fd:	c3                   	ret    

    12fe:	a1 17 03             	mov    0x317,%ax
    1301:	e9 69 ee             	jmp    0x16d
    1304:	a1 f8 01             	mov    0x1f8,%ax
    1307:	e9 63 ee             	jmp    0x16d
	...
