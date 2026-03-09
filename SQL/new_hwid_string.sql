USE [lin2world]
GO


ALTER TABLE dbo.user_data ADD
	hardware_id_string nvarchar(100) NOT NULL DEFAULT N'00000000000000000000000000000000'
GO


/****** Object:  StoredProcedure [dbo].[lin_SaveHardwareId]    Script Date: 28/09/2023 07:29:04 a.m. ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[lin_SaveHardwareIdString]
(
 @char_id INT,
 @hwid NVARCHAR(255)
)
AS

	SET NOCOUNT ON;
	UPDATE [user_data] SET [hardware_id_string] = @hwid WHERE [char_id] = @char_id

