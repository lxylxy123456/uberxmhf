import os, struct, hashlib

UCODE_DIR = '/usr/lib/firmware/intel-ucode'

def process_file(filename, content):
	offset = 0
	while content:
		assert len(content) % 1024 == 0
		data_size, = struct.unpack('I', content[28: 32])
		total_size, = struct.unpack('I', content[32: 36])
		if data_size == 0:
			data_size = 2000
			total_size = 2048
		assert len(content) >= total_size
		sha1_obj = hashlib.sha1(content[:total_size])
		msg = '{' + ', '.join(map('0x%02x'.__mod__, sha1_obj.digest())) + '},'
		end = offset + total_size
		msg += ' /* file %s, bytes% 7d -% 7d */' % (filename, offset, end)
		print('\t%s' % msg)
		offset += total_size
		content = content[total_size:]

def main():
	print('#include <xmhf.h>')
	print()
	print('unsigned char ucode_recognized_sha1s[][SHA_DIGEST_LENGTH] = {')
	for i in sorted(os.listdir(UCODE_DIR)):
		content = open(os.path.join(UCODE_DIR, i), 'rb').read()
		process_file(i, content)
	print('};')
	print()
	print('u32 ucode_recognized_sha1s_len = sizeof(ucode_recognized_sha1s) /'
		' sizeof(ucode_recognized_sha1s[0]);')

if __name__ == '__main__':
	main()

