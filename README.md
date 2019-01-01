# MQ2Bot - A Macroquest Plugin

This is the MQ2Bot plugin for macroquest2. This is a rewrite from scratch from my original MQ2Bot project.
The old plugin worked okay but had some design limitations that required a rewrite to change. 
I am happy to use this as a community forum for contributions and will keep it open source if that happens. 
If nobody contributes or only MMOBugs developers contribute then I would eventually make this source private as it neared completion. 

Details on the old project can be found at. 
- The [MQ2Bot Forums][forum] of the [MMOBugs][mmobugs] site.
- The [Wiki][wiki] is also available there.
- You can [email][email] me if you have questions and want to contribute. I am also in the MMOBugs discord.
- If you like the project and just want to [Donate][donate], that is always appreciated.

[mmobugs]: https://www.mmobugs.com/
[forum]: https://www.mmobugs.com/forums/forum132/
[wiki]: http://www.mmobugs.com/wiki/index.php/MQ2Bot
[email]: petesampras.mmobugs@gmail.com
[donate]: https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=PeteSampras%2eMMOBugs%40Gmail%2ecom&lc=US&item_name=PeteSampras&no_note=0&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHostedGuest

## Contributing guidelines

I’d love for you to help us improve this project. To help us keep this collection
high quality, I request that contributions adhere to the following guidelines.

- **Provide links to documentation** supporting the change you’re making.
  Current, canonical documentation mentioning the files being ignored is best.
  Posts from EQMule showing code/method changes are useful.
  If documentation isn’t available to support your change, do the best you can
  to explain what the files being ignored are for.

- **Explain why you’re making a change**. Even if it seems self-evident, please
  take a sentence or two to tell us why your change or addition should happen.
  It’s especially helpful to articulate why this change applies to _everyone_
  who works with the applicable technology, rather than just you or your team.

- **Please consider the scope of your change**. If your change is specific to a
  certain function please remember that it may impact other functions you might not use.
  Please try to check any other functions that use the same data as your changed function
  to ensure they still work correctly.

- **Please only modify _one template_ per pull request**. This helps keep pull
  requests and feedback focused on a specific project or technology.

In general, the more you can do to help us understand the change you’re making,
the more likely we’ll be to accept your contribution quickly.

## Commands

```
/bot on - turns the bot on
/botlist - lists all spells currently available
/bot populate spells - creates a list of your spells in the server_character.ini under [MQ2Bot]
```
