#include "ssufs-ops.h"

extern struct filehandle_t file_handle_array[MAX_OPEN_FILES];

int ssufs_allocFileHandle()
{
	for (int i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (file_handle_array[i].inode_number == -1)
		{
			return i;
		}
	}
	return -1;
}

int ssufs_create(char *filename)
{
	/* 1 */
	if(strlen(filename) > MAX_NAME_STRLEN) // extra condition. if filename exceed max length
	{
		// printf("#ERR: filename is too long : %s\n",filename);
		return -1;
	}
	
	int dup_name= open_namei(filename); //check duplicate file. 1st condition
	int i_node= ssufs_allocInode() ;	//check available space. 2nd condition
	if (dup_name  != -1) 
	{
		// printf("#ERR: find %s : %d\n",filename,dup_name);
		return -1;
	}
	// printf("#Ok: can't find %s\n",filename);
	if (i_node  == -1) 
	{
		// printf("#ERR: no space for %s\n",filename);
		return -1;
	}
	// printf("#Ok: find space for %s\n",filename);

	struct inode_t *inode_Ptr = (struct inode_t *)malloc(sizeof(struct inode_t));

	ssufs_readInode(i_node, inode_Ptr);
	inode_Ptr->status = INODE_IN_USE; //for dump print
	memcpy(inode_Ptr->name, filename, MAX_NAME_STRLEN);
	tmp->file_size = 0;
	
	ssufs_writeInode(i_node, inode_Ptr);

	free(inode_Ptr);
	return i_node;
}

void ssufs_delete(char *filename)
{
	/* 2 */
	int node_num = open_namei(filename);
	if (node_num == -1) // if there is no filename
	{
		// printf("#ERR: can't find %s to del\n", filename);
		return -1; // meaningless just quit
	}
	// printf("#OK: find %s to del : %d\n", filename, node_num);

	for (int file_handle = 0; file_handle < MAX_OPEN_FILES; file_handle++)//close if open(optional)
	{	
		if(file_handle_array[file_handle].inode_number == node_num)
		{
			// printf("#OK: find %s in handle_arr to close before del : %d\n", filename, file_handle);
			ssufs_close(file_handle); 
		}
	}
	ssufs_freeInode(node_num); // free node data (include datablock)
}

int ssufs_open(char *filename)
{
	/* 3 */
	int node_num = open_namei(filename);
	if (node_num == -1) // if there is no filename
	{
		// printf("#ERR: can't find %s to open\n", filename);
		return -1; // err
	}
	// printf("#OK: find %s to open : %d\n", filename, node_num);

	for (int file_handle = 0; file_handle < MAX_OPEN_FILES; file_handle++)
	{ // find empty handle
		if (file_handle_array[file_handle].inode_number == -1) //allocate
		{
			file_handle_array[file_handle].inode_number = node_num;
			file_handle_array[file_handle].offset = 0;
			return file_handle;
		}
	}
	return -1; //err
}

void ssufs_close(int file_handle)
{
	file_handle_array[file_handle].inode_number = -1;
	file_handle_array[file_handle].offset = 0;
}

int ssufs_read(int file_handle, char *buf, int nbytes)
{
	/* 4 */
	int handle_off = file_handle_array[file_handle].offset;
	int inode = file_handle_array[file_handle].inode_number;
	if (inode == -1 || file_handle >= MAX_OPEN_FILES || file_handle < 0)
	{
		// printf("#ERR: invalid handle and inode\n");
		return -1;
	}

	struct inode_t *inode_Ptr = (struct inode_t *)malloc(sizeof(struct inode_t));
	ssufs_readInode(inode, inode_Ptr);
	if (nbytes + handle_off > inode_Ptr->file_size)
	{ // condition 2. read & write range can't exceed file size
		// printf("#ERR: exceed file size : %d > %d bytes\n", nbytes, inode_Ptr->file_size);
		free(inode_Ptr);
		return -1;
	}

	int Start_index = handle_off / BLOCKSIZE;
	int Start_offset = handle_off % BLOCKSIZE;
	int Req_index = (handle_off + nbytes) / BLOCKSIZE;
	if((handle_off + nbytes) % BLOCKSIZE>0)
		Req_index++;
	int read_count = 0; 
	for (int j = Start_index; j < Req_index; j++) // may be meaningless
	{
	if (inode_Ptr->direct_blocks[j] == -1) 
		{
			// printf("#ERR: invalid block\n");
			free(inode_Ptr);
			return -1;
		}
	}

	for (int j = Start_index; j < Req_index && read_count < nbytes; j++)
	{
		char tmpbuf[BLOCKSIZE] = {'\0',};
		ssufs_readDataBlock(inode_Ptr->direct_blocks[j], tmpbuf); // read block[i] to tmpbuf

		// printf("#start from block_num, offset :%d %d\n", inode_Ptr->direct_blocks[j],Start_offset);
		for (int i = Start_offset; i < BLOCKSIZE && read_count < nbytes; i++)
		{

			buf[read_count] = tmpbuf[i];
			read_count++;
			//printf("#OK: read : %s %d bytes\n", buf, read_count);
		}
		Start_offset = 0; // offset in next index will be 0
	}
	// assert(read_count==nbytes);
	// printf("Curr handle, file_size: %d %d\n", file_handle_array[file_handle].offset,inode_Ptr->file_size);
	ssufs_lseek(file_handle, read_count);
	// printf("Curr handle, file_size: %d %d\n", file_handle_array[file_handle].offset,inode_Ptr->file_size);

	// printf("#OK: read : %s %d bytes\n", buf, read_count);
	free(inode_Ptr);
	return 0;
}

int ssufs_write(int file_handle, char *buf, int nbytes)
{
	/* 5 */
	int handle_off = file_handle_array[file_handle].offset;
	if ((handle_off + nbytes) > BLOCKSIZE * NUM_INODE_BLOCKS || strlen(buf)<nbytes)
	{ // condition 2. can't exceed max file size = 256(BLOCKSIZE*NUM_INODE_BLOCKS) bytes
		// printf("#ERR: exceed 256 : %d \n", nbytes);
		return -1;
	}
	int inode = file_handle_array[file_handle].inode_number;
	if (inode == -1 || file_handle >= MAX_OPEN_FILES || file_handle < 0)
	{
		// printf("#ERR: invalid handle and inode\n");
		return -1;
	}
	struct inode_t *inode_Ptr = (struct inode_t *)malloc(sizeof(struct inode_t));
	ssufs_readInode(inode, inode_Ptr);

	// printf("#OK: write : %s %d bytes\n", buf, nbytes);

	int Start_index = handle_off / BLOCKSIZE;
	int Start_offset = handle_off % BLOCKSIZE;
	int Req_index = (handle_off + nbytes) / BLOCKSIZE;
	if((handle_off + nbytes) % BLOCKSIZE>0)
		Req_index++;
	int write_count = 0;
	for (int j = Start_index; j < Req_index; j++)
	{
		char tmpbuf[BLOCKSIZE+1];

		if (inode_Ptr->direct_blocks[j] == -1)
		{
			inode_Ptr->direct_blocks[j] = ssufs_allocDataBlock();
			if (inode_Ptr->direct_blocks[j] == -1)
			{
				// printf("#ERR: no free block\n");
				free(inode_Ptr);
				return -1;
			}
		}
	}

	for (int j = Start_index; (j < Req_index && write_count < nbytes); j++)
	{
		char tmpbuf[BLOCKSIZE]={0,};

		if (Start_offset > 0)
		{
			ssufs_readDataBlock(inode_Ptr->direct_blocks[j], tmpbuf);
		}

		// printf("#start from block_num, offset :%d %d\n", inode_Ptr->direct_blocks[j],Start_offset);
		
		for (int i = Start_offset; i < BLOCKSIZE && write_count < nbytes; i++)
		{

			tmpbuf[i] = buf[write_count];
			write_count++;
		}
		Start_offset = 0;
		ssufs_writeDataBlock(inode_Ptr->direct_blocks[j], tmpbuf);
	}
	assert(write_count==nbytes);
	inode_Ptr->file_size += write_count;
	ssufs_writeInode(inode, inode_Ptr);
	ssufs_lseek(file_handle, write_count);
	free(inode_Ptr);
	return 0;
}

int ssufs_lseek(int file_handle, int nseek)
{
	int offset = file_handle_array[file_handle].offset;

	struct inode_t *tmp = (struct inode_t *)malloc(sizeof(struct inode_t));
	ssufs_readInode(file_handle_array[file_handle].inode_number, tmp);

	int fsize = tmp->file_size;

	offset += nseek;

	if ((fsize == -1) || (offset < 0) || (offset > fsize))
	{
		free(tmp);
		return -1;
	}
	
	file_handle_array[file_handle].offset = offset;
	free(tmp);

	return 0;
}
