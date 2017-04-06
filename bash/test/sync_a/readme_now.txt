


	+------------+               
	|  master    |
	+------------+
	      |
--------------+------------------+
|             |         |        |
V             V         V        V
slave1       slave2     ...     slaven





slave: ./be_slave.sh 

	性质：被同步

	slave 中作的修改，将不会被同步到master中。

	首次运行后，应该根据需要，创建修改相应的配置rsyncd.conf

	以下为需要修改的部分:
	uid/gid: 默认为当前使用者 (注意：这跟sync_secret 的用户权限相对应)

	[web]
		path: /home/wwwroot/default
		uid/gid: 同步此目录(path)的用户与组(默认为当前用户)
		hosts allow = 当前同步的网段

		

	
	修改完成后：./start_client.sh 就可以了. enjoy
	如果需要详细的日志，请添加--verbose

	出错：查看/tmp/rsyncd.log


master: ./be_master.sh
	文件变化与监控端.
	同步目录:默认为/tmp/www

	修改的部分:
	
	a. 在配置完成slave后，需要修改sync.sh
	中的RSYNC_SERVERS字段。参照里面的的格式添加slave即可。

	b. be_master.sh所传入的需要同步的根目录

	c. 如果有要过滤的文件，请传入--exclude-from=/your/file/path,
		for more info, please man rsync

	如果需要详细的日志，请添加--verbose



	如果需要添加每隔固定时间进行一步全同步：(强制将slave的数据与master进行同步)
	做一个crontab

	crontab -e 
	* * * * * /path/to/sync.sh syncnow /home/wwwroot/default >>/tmp/web_sync.log

	出错：查看/tmp/rsyncd.log


