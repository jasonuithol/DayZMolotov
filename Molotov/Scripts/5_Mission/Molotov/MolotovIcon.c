//! The molotov's inventory and hands-slot icons lean over a little; other items are untouched.
//! Orientation is in degrees: pitch, yaw, roll (as the inspect screen uses it).
static const vector MOLOTOV_ICON_TILT = "0 0 20";

class MolotovIcon
{
	static bool s_Logged;

	static void Tilt(ItemPreviewWidget preview, EntityAI obj)
	{
		if (!preview || !Molotov.Cast(obj))
			return;
		preview.SetModelOrientation(MOLOTOV_ICON_TILT);
		if (!s_Logged)
		{
			s_Logged = true;
			Print("[Molotov] icon tilted, orientation now " + preview.GetModelOrientation());
		}
	}
}

modded class Icon
{
	override void SetItemPreviewEx(bool refresh = true)
	{
		super.SetItemPreviewEx(refresh);
		MolotovIcon.Tilt(m_ItemPreview, EntityAI.Cast(m_Obj));
	}
}

modded class SlotsIconBase
{
	override void SetItemPreview()
	{
		super.SetItemPreview();
		MolotovIcon.Tilt(m_ItemPreview, m_Obj);
	}
}
