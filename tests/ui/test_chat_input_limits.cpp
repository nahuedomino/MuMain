#include <doctest.h>
#include "RmlLayoutFixture.h"

#include <RmlUi/Core/Elements/ElementFormControlInput.h>

TEST_CASE("Chat typing and paste stop at 60 Unicode characters in every theme [ui][chat]")
{
    UI::Tests::RmlLayoutFixture fixture;
    for (const char* theme : {"legacy", "classic", "modern"})
    {
        CAPTURE(theme);
        auto* document = fixture.Load(theme, "chat_input");
        REQUIRE(document != nullptr);
        auto* chat = dynamic_cast<Rml::ElementFormControlInput*>(document->GetElementById("chat_field"));
        auto* recipient = dynamic_cast<Rml::ElementFormControlInput*>(document->GetElementById("whisper_field"));
        REQUIRE(chat != nullptr);
        REQUIRE(recipient != nullptr);
        CHECK(chat->GetAttribute<int>("maxlength", -1) == 60);
        CHECK(recipient->GetAttribute<int>("maxlength", -1) == 10);
        chat->Focus();
        fixture.context->ProcessTextInput(Rml::String(120, 'a'));
        recipient->Focus();
        fixture.context->ProcessTextInput(Rml::String(20, 'b'));
        fixture.Refresh();
        CHECK(chat->GetValue().size() == 60);
        CHECK(recipient->GetValue().size() == 10);
        chat->SetValue("");
        chat->Focus();
        Rml::String unicodeText;
        for (int i = 0; i < 80; ++i)
            unicodeText += "\xf0\x9f\x98\x80";
        fixture.context->ProcessTextInput(unicodeText);
        fixture.Refresh();
        CHECK(Rml::StringUtilities::LengthUTF8(chat->GetValue()) == 60);
        CHECK(chat->GetValue().size() == 240);
        document->Close();
        fixture.Refresh();
    }
}
