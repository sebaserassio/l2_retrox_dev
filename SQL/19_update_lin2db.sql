USE [lin2db]
GO

ALTER  TABLE user_account add 
	[telefono] nvarchar(255) NOT NULL DEFAULT ""
go
