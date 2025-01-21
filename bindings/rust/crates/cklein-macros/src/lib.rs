use proc_macro::TokenStream;

#[proc_macro]
pub fn run(input: TokenStream) -> TokenStream {
    let input_tokens: proc_macro2::TokenStream = input.clone().into();

    let string = syn::parse_macro_input!(input as syn::LitStr).value();
    cklein_core::parse(&string)
        .map_err(|error| format!("Error parsing Klein code: {error}"))
        .unwrap();

    quote::quote! {
        ::cklein::run_unchecked(#input_tokens)
    }
    .into()
}
