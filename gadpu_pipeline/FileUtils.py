class FileUtils:

    def check_network_connectivity(self):
        pass


    def calculalate_file_sizse_in_MB(self, filename):
        pass


    def tail(f, lines=1, _buffer=4098):
        """Tail a file and get X lines from the end"""
        # place holder for the lines found
        lines_found = []

        # block counter will be multiplied by buffer
        # to get the block size from the end
        block_counter = -1

        # loop until we find X lines
        while len(lines_found) < lines:
            try:
                f.seek(block_counter * _buffer, os.SEEK_END)
            except IOError:  # either file is too small, or too many lines requested
                f.seek(0)
                lines_found = f.readlines()
                break

            lines_found = f.readlines()
            block_counter -= 1

        return lines_found[-lines:]


    def copy_files(self, src, dest):
        pass


    def move_files(self, src, dest):
        pass

    def delete_file_dir(self, path):
        pass