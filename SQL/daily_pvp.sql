USE [lin2world]
GO

SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

SET ANSI_PADDING ON
GO

CREATE TABLE [dbo].[user_dailypvp](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[char_id] [int] NOT NULL,
	[points] [int] NOT NULL,
	[anio] [int] NOT NULL,
	[dia] [int] NOT NULL,

 CONSTRAINT [PK_user_dailypvp] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, FILLFACTOR = 90) ON [PRIMARY]
) ON [PRIMARY]

GO

SET ANSI_PADDING OFF
GO

ALTER TABLE [dbo].[user_dailypvp] ADD  DEFAULT ((0)) FOR [char_id]
GO

ALTER TABLE [dbo].[user_dailypvp] ADD  DEFAULT ((0)) FOR [points]
GO

ALTER TABLE [dbo].[user_dailypvp] ADD  DEFAULT ((0)) FOR [anio]
GO

ALTER TABLE [dbo].[user_dailypvp] ADD  DEFAULT ((0)) FOR [dia]
GO



USE [lin2world]
GO
/****** Object:  StoredProcedure [dbo].[lin_AddPledgeSkill]    Script Date: 06/05/2023 05:02:16 a.m. ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[lin_AddDailyPvPPoint]
(
	@charId as INT,
	@points as INT,
	@anio as INT,
	@dia as INT
)
AS

SET NOCOUNT ON;

BEGIN
	IF EXISTS(SELECT * FROM [user_dailypvp] WHERE [char_id] = @charId AND [anio] = @anio AND [dia] = @dia)
	BEGIN
		UPDATE [user_dailypvp] SET [points] = @points WHERE [char_id] = @charId AND [anio] = @anio AND [dia] = @dia
	END
	ELSE
	BEGIN
		INSERT INTO [user_dailypvp] ([char_id], [points], [anio], [dia]) VALUES (@charId, @points, @anio, @dia)
	END
END

