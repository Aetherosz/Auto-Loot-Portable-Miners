#include "AutoLootPortableMinersLabel.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

UUserWidget* UAutoLootPortableMinersLabel::CreateEditorWidget_Implementation(UUserWidget* ParentWidget) const
{
	if (!ParentWidget)
	{
		return nullptr;
	}
	UAutoLootPortableMinersLabelWidget* Widget = CreateWidget<UAutoLootPortableMinersLabelWidget>(ParentWidget, UAutoLootPortableMinersLabelWidget::StaticClass());
	if (Widget)
	{
		Widget->Label = this;
	}
	return Widget;
}

TSharedRef<SWidget> UAutoLootPortableMinersLabelWidget::RebuildWidget()
{
	const TWeakObjectPtr<const UConfigProperty> WeakLabel = Label;
	return SNew(SBox)
		.Padding(FMargin(12.f, 6.f))
		[
			SNew(STextBlock)
			.Text_Lambda([WeakLabel]()
			{
				const UConfigProperty* Property = WeakLabel.Get();
				return Property ? Property->DisplayName : FText::GetEmpty();
			})
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 15))
			.ColorAndOpacity(FLinearColor::White)
			.Justification(ETextJustify::Center)
			.AutoWrapText(true)
		];
}
