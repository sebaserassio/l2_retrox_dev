USE [lin2world]
GO

/****** Object:  Table [dbo].[hwid_new_log]    Script Date: 19/08/2023 03:01:02 p.m. ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE TABLE [dbo].[hwid_new_log](
	[fecha] [datetime] NULL,
	[hwid] [nvarchar](50) NULL
) ON [PRIMARY]

GO





USE [lin2world]
GO
/****** Object:  StoredProcedure [dbo].[lin_SaveHardwareId]    Script Date: 19/08/2023 02:52:12 p.m. ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

ALTER PROCEDURE [dbo].[lin_SaveHardwareId]
(
 @char_id INT,
 @hwid NVARCHAR(50)
)
AS

	SET NOCOUNT ON;
	UPDATE [user_data] SET [long_hardware_id] = @hwid WHERE [char_id] = @char_id

	
    IF NOT EXISTS (SELECT 1 FROM hwid_new_log WHERE hwid = @hwid)
    BEGIN
        INSERT INTO hwid_new_log (fecha, hwid) VALUES (GETDATE(), @hwid);
    END



